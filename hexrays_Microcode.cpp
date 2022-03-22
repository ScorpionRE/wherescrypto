#include <ida.hpp>
#include <ua.hpp>
#include <idp.hpp>
#include <allins.hpp>
#include <segregs.hpp>


#include "common.hpp"
#include "hexrays_Microcode.hpp"
#include "DFGraph.hpp"
#include "Broker.hpp"
#include "Condition.hpp"

// TODO: 动态分配？
void MicrocodeImpl::initialize(CodeBroker& oBuilder) {
	unsigned int i;
	aRegisters.reserve(1000);

	for (i = 0; i < 1000; i++) {
		aRegisters.push_back(oBuilder->NewRegister(i));
	}

	dwMaxCallDepth = oBuilder->MaxCallDepth();
}

// microcode应该没有PC寄存器
// TODO: 某些寄存器如eax映射mreg_t为8-11如何处理？？？
// 实际上只是假设，实际情况下microcode中mreg_t为一个int值
DFGNode MicrocodeImpl::GetRegister(CodeBroker& oBuilder, unsigned long lpInstructionAddress, mreg_t bReg) {
	
	if (bReg > 1000 || bReg < 0) {
		wc_debug("unsupported regnumber: %d\n", bReg);
		return nullptr;
	}
	return aRegisters[bReg];
}

processor_status_t MicrocodeImpl::SetRegister(CodeBroker& oBuilder, unsigned long lpInstructionAddress,  mreg_t bReg, DFGNode& oNode)
{
	if (bReg > 1000 || bReg < 0) {
		wc_debug("unsupported regnumber: %d\n", bReg);
		return PROCESSOR_STATUS_INTERNAL_ERROR;
	}
	aRegisters[bReg] = oNode;
	return PROCESSOR_STATUS_OK;
}


DFGNode MicrocodeImpl::GetOperandShift(CodeBroker& oBuilder, DFGNode& oBaseNode, DFGNode& oShift, mcode_t opcode, bool bSetFlags)
{
	switch(opcode) {
	case m_shl:
		break;
	case m_shr:
	case m_sar: 
		oShift = oBuilder->NewMult(oShift, oBuilder->NewConstant(-1));
		break;
	}

	if (bSetFlags) {
		SetFlag(FLAG_MOP_SHIFT, oBaseNode, oShift);
	}

	return oBuilder->NewShift(oBaseNode, oShift);
}

processor_status_t MicrocodeImpl::SetOperand(CodeBroker& oBuilder, const mop_t& stOperand, unsigned long lpInstructionAddress, DFGNode oNode, bool bSetFlags)
{
	processor_status_t eStatus;
	switch (stOperand.t) {
	case mop_r: {
		eStatus = SetRegister(oBuilder, lpInstructionAddress, stOperand.r, oNode);
		break;
	}
	case mop_d: {
		eStatus = PROCESSOR_STATUS_OK;
		break;
	}
	case mop_S: {
		// member_t* stVar = stOperand.s->mba->find_stkvar(stOperand.s->off);
		
		
	}
	default: {
		eStatus = PROCESSOR_STATUS_OK;
		break;
	}
		
	}
	return eStatus;

}

DFGNode MicrocodeImpl::GetOperand(CodeBroker& oBuilder, const mop_t& stOperand, unsigned long *&lpNextAddress, unsigned long lpInstructionAddress, bool bSetFlags, minsn_t *&mNextInstruction)
{
	switch (stOperand.t) {
	case mop_r: {
		DFGNode oReg = GetRegister(oBuilder, lpInstructionAddress, stOperand.r);
		if (bSetFlags) {
			SetFlag(FLAG_MOP_ADD, oReg, oBuilder->NewConstant(0));
		}
		return oReg;
	}
	case mop_d: {
		mNextInstruction = stOperand.d;
		break;
	}
	case mop_a:  //TODO：？？？
		// return oBuilder->NewConstant(stOperand.a);

	case mop_n:
		return oBuilder->NewConstant(stOperand.nnn->value);
	
	case mop_S:
		// member_t* stVar = stOperand.s->mba->get_stkvar(stOperand.s->off);
		// return oBuilder->NewConstant(stVar);

	case mop_v: {
		*lpNextAddress = stOperand.g;
		int nsucc = currentBlock->serial;
		for (int i = 0; i < currentFuncMicrocode.value()->qty ; i++) {
			mblock_t* nextBB = currentFuncMicrocode.value()->natural[i];
			if (nextBB->start == stOperand.g) {
				mNextInstruction = nextBB->head;
				*lpNextAddress = mNextInstruction->ea;
				currentBlock = nextBB;
				return oBuilder->NewConstant(*lpNextAddress);
			}
		}

		return oBuilder->NewConstant(*lpNextAddress);
		break;
	}
	case mop_b:
		currentBlock = currentFuncMicrocode.value()->natural[stOperand.b];
		mNextInstruction = currentBlock->head;
		*lpNextAddress = mNextInstruction->ea;
		return oBuilder->NewConstant(*lpNextAddress);

	case mop_z:
		break;
	default:
		wc_debug("[-] unsupported operand type: %d\n", stOperand.t);
		break;
	}
	return DFGNode();
}

processor_status_t MicrocodeImpl::JumpToNode(CodeBroker& oBuilder, unsigned long*& lpNextAddress,  unsigned long lpInstructionAddress, DFGNode oAddress)
{
	unsigned long lpTarget(0);

	if (NODE_IS_CONSTANT(oAddress)) {
		lpTarget = oAddress->toConstant()->dwValue & ~1;
		// lpTarget = oAddress->toConstant()->dwValue;
	_jump:
		API_LOCK();
		segment_t* lpSegment = getseg(lpTarget);
		qstring szSegmentName;
		get_segm_name(&szSegmentName, lpSegment);
		API_UNLOCK();

		PopCallStack(lpTarget);

		if (lpSegment != NULL & szSegmentName != "extern" && aCallStack.size() < dwMaxCallDepth) { //  && aCallStack.size() < dwMaxCallDepth
			*lpNextAddress = lpTarget;
		}
		else {
			DFGNode oCallNode = oBuilder->NewCall(lpTarget, GetRegister(oBuilder, lpInstructionAddress, 7));  // TODO: register 0???
			API_LOCK();
			func_t* lpFunction = get_func(lpTarget);
			bool bFunctionReturns = true;
			if (
				lpFunction != NULL &&
				(lpFunction->start_ea & ~1) == (lpTarget & ~1) &&
				!func_does_return(lpTarget & ~1)
				) {
				bFunctionReturns = false;
			}
			API_UNLOCK();
			if (!bFunctionReturns) {
				wc_debug("[*] CALL at address 0x%x does not return, stopping analysis\n", lpInstructionAddress);
				return PROCESSOR_STATUS_DONE;
			}
			SetRegister(oBuilder, lpInstructionAddress, 7, oCallNode);
			// TODO: 函数返回信息

			DFGNode oLr = oBuilder->NewConstant(*lpNextAddress);
			*lpNextAddress = oLr->toConstant()->dwValue & ~1;
			PopCallStack(*lpNextAddress);

			/*DFGNode oLr = GetRegister(oBuilder, lpInstructionAddress, 14);
			if (!NODE_IS_CONSTANT(oLr) && !(NODE_IS_REGISTER(oLr) && oLr->toRegister()->bRegister == 14)) {
				wc_debug("[-] loading of non-constant expression %s into PC is not supported @ 0x%x\n", oLr->expression(2).c_str(), lpInstructionAddress);
				return PROCESSOR_STATUS_INTERNAL_ERROR;
			}
			if (NODE_IS_REGISTER(oLr)) {
				goto _load_lr;
			}
			else {
				*lpNextAddress = oLr->toConstant()->dwValue & ~1;
				PopCallStack(*lpNextAddress);
			}*/
		}
		return PROCESSOR_STATUS_OK;
	}
	else if (NODE_IS_ADD(oAddress)) {
		DFGNode oLoadAddress = *oAddress->toLoad()->aInputNodes.begin();
		if (NODE_IS_CONSTANT(oLoadAddress)) {
			API_LOCK();
			lpTarget = get_dword(oLoadAddress->toConstant()->dwValue) & ~1;
			API_UNLOCK();
			goto _jump;
		}
	}
	if (NODE_IS_REGISTER(oAddress) && oAddress->toRegister()->bRegister == 14) {
	_load_lr:
		wc_debug("[+] loading %s into PC @ 0x%x\n", oAddress->expression(2).c_str(), lpInstructionAddress);
		if (aCallStack.size() > 0) {
			wc_debug("[-] call stack not empty :(\n");

			//std::list<unsigned long>::reverse_iterator itS;
			//int i;
			//for (i = aCallStack.size() - 1, itS = aCallStack.rbegin(); itS != aCallStack.rend(); itS++, i--) {
			//	wc_debug("   [*] %2d 0x%x\n", i, *itS);
			//}
			//return PROCESSOR_STATUS_INTERNAL_ERROR;
		}
		return PROCESSOR_STATUS_DONE;
	}
	else {
		wc_debug("[-] loading of non-constant expression %s into PC is not supported @ 0x%x\n", oAddress->expression(2).c_str(), lpInstructionAddress);
		return PROCESSOR_STATUS_INTERNAL_ERROR;
	}
	return processor_status_t();
}

void MicrocodeImpl::PushCallStack(unsigned long lpAddress)
{
	aCallStack.push_back(lpAddress);
}

// microcode中ret用数据流分析找到要返回的地址

void MicrocodeImpl::PopCallStack(unsigned long lpAddress)
{
	std::list<unsigned long>::reverse_iterator it;
	/*
	 * We traverse down the stack to find the entry
	 * since we found examples in the wild where the go-to-begin
	 * is done through a BL, discarding LR. This poisons our aCallStack.
	 */
	for (it = aCallStack.rbegin(); it != aCallStack.rend(); it++) {
		if (*it == lpAddress) {
			aCallStack.erase((++it).base(), aCallStack.end());
			break;
		}
	}
}

bool MicrocodeImpl::isSetConditional(minsn_t* mInstruction) {
	switch (mInstruction->opcode)
	{
	case m_seta:
	case m_setae:
	case m_setb:
	case m_setbe:
	case m_setg:
	case m_setge:
	case m_setl:
	case m_setle:
	case m_setnz:
	case m_setz:
	case m_jz:
	case m_jnz:
	case m_ja:
	case m_jae:
	case m_jb:
	case m_jbe:
	case m_jg:
	case m_jge:
	case m_jl:
	case m_jle:
	case m_jcnd:
		return true;
	default:
		return false;
	}
}

// 判断是否有子指令，有则有则调用instruction继续分析，得到结果？？？


// 一条一条分析microcode指令
processor_status_t MicrocodeImpl::instruction(CodeBroker& oBuilder, unsigned long *lpNextAddress, unsigned long lpAddress, minsn_t* mInstruction, minsn_t*& mNextInstruction) {

	unsigned int i;
	unsigned int dwRegisterNo;
	int dwInstructionSize;
	udc_filter_t *udc;
	

  	if (!currentFuncMicrocode.has_value()) {
		if (!GenMicrocode(lpAddress)) {
			return PROCESSOR_STATUS_INTERNAL_ERROR;
		}
		currentBlock = currentFuncMicrocode.value()->blocks->nextb;
		mInstruction = currentBlock->head;
		currentFuncMicrocode.value()->build_graph();
	}


	if (!currentBlock || mInstruction == nullptr) {
		wc_debug("[-] NULL \n");
		return PROCESSOR_STATUS_OK;
	}
	
	if(mNextInstruction = mInstruction->next)
		*lpNextAddress = mNextInstruction->ea;
	
	if (mInstruction == currentBlock->tail) {
		// TODO: basic block 选择
		mblock_t* nextBB;
		int nsucc;
		switch (currentBlock->type)
		{
		case BLT_STOP:
			return PROCESSOR_STATUS_DONE;
		case BLT_0WAY:
			return PROCESSOR_STATUS_OK;
		case BLT_1WAY:
			nextBB = currentBlock->nextb;
			break;
		case BLT_2WAY:
			nsucc = currentBlock->nsucc();
			for (int i = 0; i < nsucc; i++) {
				nextBB = currentFuncMicrocode.value()->natural[currentBlock->succ(i)];
				if (nextBB->start == currentBlock->end) {
					break;
				}
			}
			currentBlock = nextBB;
			break;
			
		case BLT_NWAY: // TODO: switch idiom
			nsucc = currentBlock->nsucc();
			for (int i = 0; i < nsucc; i++) {
				nextBB = currentFuncMicrocode.value()->natural[currentBlock->succ(i)];
				if (nextBB->start == currentBlock->end) {
					break;
				}
			}
			break;
				
		default:
			nextBB = currentFuncMicrocode.value()->natural[currentBlock->serial + 1];
			
			
			break;
		}
		if (nextBB->type == BLT_STOP) {
			currentFuncMicrocode.reset();
			return PROCESSOR_STATUS_DONE;
		}
		mNextInstruction = nextBB->head;
		*lpNextAddress = mNextInstruction->ea;
		currentBlock = nextBB;
		
	}
	

	DFGNode oConditionNode;
	graph_process_t eVerdict = GRAPH_PROCESS_CONTINUE;
	Condition oCondition;
	switch (mInstruction->opcode) { // mcode_t op操作码

		
	case m_setz:  // 0x21 Z Equal 
	case m_setnz: // 0x20 !Z Not equal
	case m_setae: // 0x22  !C Above or Equal
	case m_setb:  // 0x23  C  below 
	case m_seto:
	case m_setg: // 0x26  SF=OF & ZF=0  Greater
	case m_setle: // 0x29 SF!=OF | ZF=1 Less or Equal
	case m_setge: // 0x27 SF=OF Greater or Equal
	case m_setl:  // 0x28 SF!=OF less
	case m_seta:  // 0x24   !C & !Z  Above
	case m_setbe: // 0x25  C | Z  Below or Equal
	{
		DFGNode oNode1 = GetOperand(oBuilder, mInstruction->l, *&lpNextAddress, lpAddress, false,*&mNextInstruction);
		DFGNode oNode2 = GetOperand(oBuilder, mInstruction->r, *&lpNextAddress,lpAddress, false, *&mNextInstruction);
		oNode2 = oBuilder->NewMult(oNode2, oBuilder->NewConstant(-1));
		DFGNode oAdd = oBuilder->NewAdd(oNode1, oNode2);
		SetFlag(FLAG_MOP_ADD, oNode1, oNode2);
		if (oNode1 == oNode2) {
			processor_status_t eStatus = SetOperand(oBuilder, mInstruction->d,lpAddress, oBuilder->NewConstant(0),false);
			if (eStatus != PROCESSOR_STATUS_OK) {
				return eStatus;
			}
		}
		else {
			processor_status_t eStatus = SetOperand(oBuilder,  mInstruction->d, lpAddress,oAdd,false);
			if (eStatus != PROCESSOR_STATUS_OK) {
				return eStatus;
			}
		}
		break;
	}
	case m_sets: {
		DFGNode oNode1 = GetOperand(oBuilder, mInstruction->l, *&lpNextAddress, lpAddress, false, *&mNextInstruction);
		SetFlagN(FLAG_MOP_NEG, oNode1);
		break;
	}
	case m_setp:  // unordered 
		break;
	
		
	case m_jcnd: {
		DFGNode oL = GetOperand(oBuilder, mInstruction->l,*&lpNextAddress, lpAddress, true, *& mNextInstruction);
		Condition oCondition(Condition::create(oL, OPERATOR_NEQ, oBuilder->NewConstant(0)));  //相当于不为0（即为真）则跳转
		DFGNode oD = GetOperand(oBuilder, mInstruction->d, *&lpNextAddress, lpAddress, false, *& mNextInstruction);

		eVerdict = oBuilder->IntroduceCondition(oCondition, *lpNextAddress);
		if (eVerdict == GRAPH_PROCESS_SKIP) {
			goto _skip;
		}
		else if (eVerdict == GRAPH_PROCESS_INTERNAL_ERROR) {
			return PROCESSOR_STATUS_INTERNAL_ERROR;
		}
		break;

	}

	case m_jz: 
	case m_jnz:
	case m_jae:
	case m_jb: 
	case m_ja: 
	case m_jbe:
	case m_jg:
	case m_jge:
	case m_jl:
	case m_jle: {
		DFGNode oNode1 = GetOperand(oBuilder, mInstruction->l, *&lpNextAddress, lpAddress, false, *&mNextInstruction);
		DFGNode oNode2 = GetOperand(oBuilder, mInstruction->r, *&lpNextAddress, lpAddress, false, *&mNextInstruction);
		oNode2 = oBuilder->NewMult(oNode2, oBuilder->NewConstant(-1));
		DFGNode oAdd = oBuilder->NewAdd(oNode1, oNode2);
		SetFlag(FLAG_MOP_ADD, oNode1, oNode2);
		DFGNode oD = GetOperand(oBuilder, mInstruction->d, *&lpNextAddress, lpAddress, false, *&mNextInstruction);
		switch (mInstruction->opcode) {
		case m_jz:
		case m_jnz: {
			Condition oCondition(Condition::create(oAdd, mInstruction->opcode == m_jz ? OPERATOR_EQ : OPERATOR_NEQ, oBuilder->NewConstant(0)));
			break;
		}
		case m_jae:
		case m_jb: {
			Condition oCondition(Condition::create(oAdd, mInstruction->opcode == m_jae ? OPERATOR_UGE : OPERATOR_ULT, oBuilder->NewConstant(0)));
			break;
		}
		case m_ja:
		case m_jbe: {
			Condition oCondition(Condition::create(oAdd, mInstruction->opcode == m_ja ? OPERATOR_UGT : OPERATOR_ULE, oBuilder->NewConstant(0)));
			break;
		}
		case m_jge:
		case m_jl: {
			Condition oCondition(Condition::create(oAdd, mInstruction->opcode == m_jge ? OPERATOR_GE : OPERATOR_LT, oBuilder->NewConstant(0)));
			break;
		}
		case m_jg:
		case m_jle: {
			Condition oCondition(Condition::create(oAdd, mInstruction->opcode == m_jg ? OPERATOR_GT : OPERATOR_LE, oBuilder->NewConstant(0)));
			break;
		}
		}
		eVerdict = oBuilder->IntroduceCondition(oCondition, *lpNextAddress);
		if (eVerdict == GRAPH_PROCESS_SKIP) {
			goto _skip;
		}
		else if (eVerdict == GRAPH_PROCESS_INTERNAL_ERROR) {
			return PROCESSOR_STATUS_INTERNAL_ERROR;
		}
	}
	case m_jtbl:
		break;
	

	case m_ldx: 
	case m_ijmp: {  // TODO: ijmp
		
		
		processor_status_t eStatus;
		DFGNode oLoad;
		DFGNode oReg = GetOperand(oBuilder, mInstruction->l, *&lpNextAddress,lpAddress, false, *&mNextInstruction);
		DFGNode oOff = GetOperand(oBuilder, mInstruction->r, *&lpNextAddress, lpAddress, false, *&mNextInstruction);
		oReg = oBuilder->NewAdd(oReg, oOff);
		oLoad = oBuilder->NewLoad(oReg);
		if ((eStatus = SetRegister(oBuilder, lpAddress, mInstruction->l.r, oReg)) != PROCESSOR_STATUS_OK) {
			return eStatus;
		}
		break;

	}

	case m_stx: {
		DFGNode oData = GetOperand(oBuilder,  mInstruction->l , *&lpNextAddress, lpAddress, false, *&mNextInstruction);
		processor_status_t eStatus;
		DFGNode oReg = GetOperand(oBuilder, mInstruction->r , *&lpNextAddress, lpAddress, false, *&mNextInstruction); // 一般为ds,cs
		DFGNode oOff = GetOperand(oBuilder, mInstruction->d, *&lpNextAddress, lpAddress, false, *&mNextInstruction);
		oReg = oBuilder->NewAdd(oReg, oOff);
		oBuilder->NewStore(oData, oReg);
		if ((eStatus = SetRegister(oBuilder, lpAddress, mInstruction->r.r , oReg)) != PROCESSOR_STATUS_OK) {
			return eStatus;
		}

		break;
	}


	
	case m_fmul:
	case m_mul: { // 0x0E mul l,r,d  l*r->d
		DFGNode oNode1, oNode2;
		oNode1 = GetOperand(oBuilder, mInstruction->l, *&lpNextAddress, lpAddress, false, * &mNextInstruction);
		oNode2 = GetOperand(oBuilder, mInstruction->r, *&lpNextAddress, lpAddress, false, *&mNextInstruction);
		DFGNode oMult = oBuilder->NewMult(oNode1, oNode2);
		processor_status_t eStatus = SetRegister(oBuilder, lpAddress,  mInstruction->d.r,oMult);
		if (eStatus != PROCESSOR_STATUS_OK) {
			return eStatus;
		}
		// TODO: 但是为什么setconditional时需要setflag？？
		if (isSetConditional(mInstruction)) {
			SetFlag(FLAG_MOP_MULT, oNode1, oNode2);
		}
		break;
	}
	case m_add:
	case m_fadd: {
		DFGNode oNode1, oNode2;
		oNode1 = GetOperand(oBuilder, mInstruction->l, *&lpNextAddress, lpAddress, false, *&mNextInstruction);
		oNode2 = GetOperand(oBuilder, mInstruction->r, *&lpNextAddress, lpAddress, false, *&mNextInstruction);
		DFGNode oAdd = oBuilder->NewAdd(oNode1, oNode2);
		// TODO: 传进去前是否都需要判断是否为子指令
		processor_status_t eStatus = SetRegister(oBuilder, lpAddress, mInstruction->d.r, oAdd);
		if (eStatus != PROCESSOR_STATUS_OK) {
			return eStatus;
		}
		break;

	}
	case m_sub: 
	case m_fsub: {
		DFGNode oNode1, oNode2;
		oNode1 = GetOperand(oBuilder, mInstruction->l, *&lpNextAddress,lpAddress, false, *&mNextInstruction);
		oNode2 = GetOperand(oBuilder, mInstruction->r, *&lpNextAddress, lpAddress, false, *&mNextInstruction);
		oNode2 = oBuilder->NewMult(oNode2, oBuilder->NewConstant(-1));
		DFGNode oAdd = oBuilder->NewAdd(oNode1, oNode2);
		processor_status_t eStatus = SetRegister(oBuilder, lpAddress, mInstruction->d.r, oAdd);
		if (eStatus != PROCESSOR_STATUS_OK) {
			return eStatus;
		}
		break;

	}
	case m_cfadd: {  // TODO: calculate carry flag
		DFGNode oNode1, oNode2;
		oNode1 = GetOperand(oBuilder, mInstruction->l, *&lpNextAddress, lpAddress, false, *&mNextInstruction);
		oNode2 = GetOperand(oBuilder, mInstruction->r, *&lpNextAddress, lpAddress, false, *&mNextInstruction);

		
		
		break;
	}

	case m_ofadd: {
		break;
	}
	case m_cfshl: {
		break;
	}
	case m_fdiv:
	case m_udiv: { // TODO: unsigned?
		break;
	}
	case m_sdiv: { // signed?
		break;
	}
	case m_umod: {
		break;
	}
	case m_smod: {
		break;
	}
		

	case m_ldc: {
		break;
	}
	
	
	case m_mov: {
		DFGNode oNode = GetOperand(oBuilder, mInstruction->l, *&lpNextAddress, lpAddress, !!isSetConditional(mNextInstruction), *&mNextInstruction);
		processor_status_t eStatus = SetOperand(oBuilder, mInstruction->d, lpAddress, oNode, isSetConditional(mNextInstruction));
		// processor_status_t eStatus = SetRegister(oBuilder, lpAddress,mInstruction->d.r , oNode);
		if (eStatus != PROCESSOR_STATUS_OK) {
			return eStatus;
		}
		break;
	}
	case m_fneg:
	case m_neg: {
		DFGNode oNode = oBuilder->NewMult(GetOperand(oBuilder, mInstruction->l, *&lpNextAddress,lpAddress, !!isSetConditional(mInstruction), *&mNextInstruction), oBuilder->NewConstant(-1));
		processor_status_t eStatus = SetRegister(oBuilder, lpAddress, mInstruction->d.r, oNode);
		if (eStatus != PROCESSOR_STATUS_OK) {
			return eStatus;
		}
		break;
	}
	case m_lnot: { // TODO:逻辑非，与1作and？？
		DFGNode oNode = oBuilder->NewAnd(GetOperand(oBuilder, mInstruction->l, *&lpNextAddress,lpAddress, !!isSetConditional(mInstruction), *&mNextInstruction), oBuilder->NewConstant(1));
		processor_status_t eStatus = SetRegister(oBuilder, lpAddress,  mInstruction->d.r, oNode);
		if (eStatus != PROCESSOR_STATUS_OK) {
			return eStatus;
		}
		break;
		
	}
	case m_bnot: {
		DFGNode oNode = oBuilder->NewXor(GetOperand(oBuilder, mInstruction->l, *&lpNextAddress, lpAddress, !!isSetConditional(mInstruction), *&mNextInstruction), oBuilder->NewConstant(0xffffffff));
		processor_status_t eStatus = SetRegister(oBuilder, lpAddress,  mInstruction->d.r, oNode);
		if (eStatus != PROCESSOR_STATUS_OK) {
			return eStatus;
		}
		break;
	}



	case m_push: { // TODO: push and pop???
		ea_t dwSpec;
		bool bPost = false;
		bool bIncrement = false;
		bool bWriteback = true;
		int dwIncrement = 0;

		DFGNode oOperand = GetRegister(oBuilder, lpAddress, mInstruction->l.r);

		



		break;

	}
	case m_pop: {
		break;
	}
	case m_and: { 
		DFGNode oSource;
		DFGNode oNode1, oNode2;
		bool bUpdateFlags = (isSetConditional(mNextInstruction));

		oNode1 = GetOperand(oBuilder, mInstruction->l, *&lpNextAddress, lpAddress, bUpdateFlags, *&mNextInstruction);
		oNode2 = GetOperand(oBuilder, mInstruction->r, *&lpNextAddress,lpAddress, bUpdateFlags, *&mNextInstruction);
		oSource = oBuilder->NewAnd(oNode1, oNode2); 
		processor_status_t eStatus = SetOperand(oBuilder, mInstruction->d, lpAddress,oSource, FLAG_MOP_BITWISE_AND);
		if (eStatus != PROCESSOR_STATUS_OK) {
			return eStatus;
		}
		if (bUpdateFlags) {
			SetFlag(FLAG_MOP_BITWISE_AND, oNode1, oNode2);
		}
		break;
	}

	case m_xor: {
		DFGNode oSource;
		DFGNode oNode1, oNode2;
		bool bUpdateFlags = (isSetConditional(mNextInstruction));
		
		oNode1 = GetOperand(oBuilder, mInstruction->l, *&lpNextAddress, lpAddress, bUpdateFlags, *&mNextInstruction);
		oNode2 = GetOperand(oBuilder, mInstruction->r, *&lpNextAddress, lpAddress, bUpdateFlags, *&mNextInstruction);
		
		oSource = oBuilder->NewXor(oNode1, oNode2); 

		processor_status_t eStatus = SetOperand(oBuilder, mInstruction->d, lpAddress, oSource, FLAG_MOP_BITWISE_XOR);
		if (eStatus != PROCESSOR_STATUS_OK) {
			return eStatus;
		}
		if (bUpdateFlags) {
			SetFlag(FLAG_MOP_BITWISE_XOR, oNode1, oNode2);
		}

		break;
	}

	case m_or: {
		DFGNode oSource;
		DFGNode oNode1, oNode2;
		bool bUpdateFlags = (isSetConditional(mNextInstruction));
		oNode1 = GetOperand(oBuilder,  mInstruction->l, *&lpNextAddress,lpAddress, bUpdateFlags, *&mNextInstruction);
		oNode2 = GetOperand(oBuilder, mInstruction->r, *&lpNextAddress, lpAddress, bUpdateFlags, *&mNextInstruction);

		oSource = oBuilder->NewOr(oNode1, oNode2); 


		processor_status_t eStatus = SetOperand(oBuilder, mInstruction->d, lpAddress, oSource, FLAG_MOP_BITWISE_OR);
		if (eStatus != PROCESSOR_STATUS_OK) {
			return eStatus;
		}
		if (bUpdateFlags) {
			SetFlag(FLAG_MOP_BITWISE_OR, oNode1, oNode2);
		}
		break;
	}

	case m_shl: {
		DFGNode oNode1, oNode2;
		oNode1 = GetRegister(oBuilder, lpAddress, mInstruction->l.r);
		oNode2 = GetRegister(oBuilder, lpAddress, mInstruction->r.r);
		processor_status_t eStatus;
		DFGNode oSource;
		oSource = oBuilder->NewShift(oNode1, oNode2);


		SetFlag(FLAG_MOP_SHIFT, oNode1, oNode2);
		// eStatus = SetRegister(oBuilder, lpAddress,  dwRegisterNo, oSource);

		if (eStatus != PROCESSOR_STATUS_OK) {
			return eStatus;
		}
		break;
	}
	case m_shr: {
		//TODO: 右移
		break;
	}
	case m_sar: {
		DFGNode oNode1, oNode2;
		oNode1 = GetRegister(oBuilder, lpAddress, mInstruction->l.r);
		oNode2 = GetRegister(oBuilder, lpAddress, mInstruction->r.r);
		processor_status_t eStatus;
		DFGNode oSource;
		oSource = oBuilder->NewRotate(oNode1, oNode2);


		SetFlag(FLAG_MOP_SHIFT, oNode1, oNode2);
		eStatus = SetRegister(oBuilder, lpAddress,  dwRegisterNo, oSource);
		if (eStatus != PROCESSOR_STATUS_OK) {
			return eStatus;
		}
		break;
	}
	case m_xdu: {
		break;
	}

	case m_xds: {
		break;
	}
	case m_low: {
		break;
	}
	case m_high: {
		break;
	}

	case m_goto: {
		DFGNode oAddress = GetOperand(oBuilder, mInstruction->l, *&lpNextAddress,lpAddress, false, *&mNextInstruction);
		return JumpToNode(oBuilder, *&lpNextAddress,  lpAddress, oAddress);
	}
	case m_call: {
		DFGNode oAddress = GetOperand(oBuilder, mInstruction->l, *&lpNextAddress, lpAddress, false, *&mNextInstruction);
		return JumpToNode(oBuilder, *&lpNextAddress,  lpAddress, oAddress);
	
	}
	case m_icall: {

	}

	case m_ret: { //TODO: call 保存的返回值在哪
		DFGNode oAddress = GetRegister(oBuilder, lpAddress, 14);
		return JumpToNode(oBuilder, *&lpNextAddress, lpAddress, oAddress);
	}
	

	case m_f2f: {
		break;
	}
	case m_f2i: {
		break;
	}
	case m_i2f: {
		break;
	}
	case m_f2u: {
		break;
	}
	case m_u2f: {
		break;
	}
	case m_nop: 
		break;
	case m_und: {
		break;
	}
	case m_ext: {
		break;
	}
	default:
		wc_debug("unhandled instruction type: %d at 0x%x", mInstruction->opcode, lpAddress);
		return PROCESSOR_STATUS_INTERNAL_ERROR;
	}


_skip:
	return PROCESSOR_STATUS_OK;
}
 
// TODO:改进
bool MicrocodeImpl::ShouldClean(DFGNode& oNode) {
	if (NODE_IS_STORE(oNode)) {
		/*
		 * stores on the stack are temporary
		 * and should be removed from the graph if their outputs remain unused
		 */
		DFGNode oMemoryAddress = *++oNode->aInputNodes.begin();
		if (NODE_IS_REGISTER(oMemoryAddress) && oMemoryAddress->toRegister()->bRegister == 13) {
			/* store to SP */
			return true;
		}
		else if (NODE_IS_ADD(oMemoryAddress)) {
			std::list<DFGNode>::iterator it;
			/* store to SP+x */
			for (it = oMemoryAddress->aInputNodes.begin(); it != oMemoryAddress->aInputNodes.end(); it++) {
				if (NODE_IS_REGISTER(*it) && (*it)->toRegister()->bRegister == 13) {
					return true;
				}
			}
		}
		/* any other store should be considered to be an output value */
		return false;
	}
	return true;
}

bool MicrocodeImpl::GenMicrocode(unsigned long lpAddress) {
	

	// generate microcode
	hexrays_failure_t hf;
	mba_ranges_t mbr = mba_ranges_t();
	func_t* fn = get_func(lpAddress);


	if (fn == NULL)
	{
		wc_debug("Please position the cursor within a function");
		return false;
	} 

	ea_t ea1 = fn->start_ea;
	ea_t ea2 = fn->end_ea;
	hf = hexrays_failure_t();

	flags_t F = get_flags(ea1);
	if (!is_code(F))
	{
		wc_debug("The selected range must start with an instruction");
		return false;
	}
	mbr.ranges.push_back(range_t(ea1, ea2));
	currentFuncMicrocode = gen_microcode(mbr, &hf, NULL, DECOMP_WARNINGS, MMAT_GENERATED);
	if (currentFuncMicrocode.value() == NULL)
	{
		wc_debug("%a: %s", hf.errea, hf.desc().c_str());
		
	}

	msg("Successfully generated microcode for %a..%a\n", ea1, ea2);
	// vd_printer_t vp;
	// mba->print(vp);

	// basic blocks

	//int qty = mba->qty;
	//mblock_t* blocks = mba->blocks->nextb;
	//msg("%d basic blocks", qty);


	//// instructions 
	//msg("No %d basic block", blocks->serial);
	//minsn_t* ins = blocks->head;

	//return ins;
	//msg("instructs opcode:%x, l:%d.%d  , r:%d.%d , d: %d.%d ", ins->opcode, ins->l.r, ins->l.size, ins->r.r, ins->r.size, ins->d.r, ins->d.size);



	return true;

}

Processor MicrocodeImpl::Migrate(DFGraph oGraph) {
	// genMicrocode();

	Microcode lpFork(Microcode::create(*this));
	std::vector<DFGNode>::iterator it;



	lpFork->aRegisters.clear();
	lpFork->aRegisters.reserve(aRegisters.size());
	for (it = aRegisters.begin(); it != aRegisters.end(); it++) {
		lpFork->aRegisters.push_back(oGraph->FindNode((*it)->dwNodeId));
	}

	if (lpFork->oCarryFlag) lpFork->oCarryFlag = lpFork->oCarryFlag->Migrate(oGraph);
	if (lpFork->oOverflowFlag) {
		if (lpFork->oOverflowFlag == oCarryFlag) { lpFork->oOverflowFlag = lpFork->oCarryFlag; }
		else { lpFork->oOverflowFlag = lpFork->oOverflowFlag->Migrate(oGraph); }
	}
	if (lpFork->oZeroFlag) {
		if (lpFork->oZeroFlag == oCarryFlag) { lpFork->oZeroFlag = lpFork->oCarryFlag; }
		else if (lpFork->oZeroFlag == oOverflowFlag) { lpFork->oZeroFlag = lpFork->oOverflowFlag; }
		else { lpFork->oZeroFlag = lpFork->oZeroFlag->Migrate(oGraph); }
	}
	if (lpFork->oNegativeFlag) {
		if (lpFork->oNegativeFlag == oCarryFlag) { lpFork->oNegativeFlag = lpFork->oCarryFlag; }
		else if (lpFork->oNegativeFlag == oOverflowFlag) { lpFork->oNegativeFlag = lpFork->oOverflowFlag; }
		else if (lpFork->oNegativeFlag == oZeroFlag) { lpFork->oNegativeFlag = lpFork->oZeroFlag; }
		else { lpFork->oNegativeFlag = lpFork->oNegativeFlag->Migrate(oGraph); }
	}

	return Processor::typecast(lpFork);
}

DFGNode flag_mop_t::Carry(CodeBroker& oBuilder)
{
	DFGNode oCarryNode(nullptr);
	switch (eOperation) {
	case FLAG_MOP_ADD:
		oBuilder->NewAdd(oNode1, oNode2, &oCarryNode);
		break;
	case FLAG_MOP_SHIFT:
		oBuilder->NewShift(oNode1, oNode2, &oCarryNode);
		break;
	}
	return oCarryNode;
}

Condition flag_mop_t::ConditionalInstruction(CodeBroker& oBuilder, mcode_t opcode)
{
	switch (eOperation) {
	case FLAG_MOP_ADD:
		return ConditionalInstructionAdd(oBuilder, opcode);
	case FLAG_MOP_SHIFT:
		return ConditionalInstructionShift(oBuilder, opcode);
	case FLAG_MOP_MULT:
	case FLAG_MOP_BITWISE_AND:
	case FLAG_MOP_BITWISE_OR:
	case FLAG_MOP_BITWISE_XOR:
		return ConditionalInstructionBitwise(oBuilder, opcode);
	}
}

Condition flag_mop_t::ConditionalInstructionAdd(CodeBroker& oBuilder, mcode_t opcode)
{
	operator_t eOperator;
	DFGNode oLeftNode, oRightNode;
	switch (opcode) {
	case m_setz:
	case m_setnz:
	case m_setae: // 0x22  !C Above or Equal
	case m_setb:  // 0x23  C  below
	case m_seta: // 0x24   !C & !Z  Above
	case m_setbe: // 0x25  C | Z  Below or Equal
	case m_setge: // 0x27 SF=OF Greater or Equal
	case m_setl:  // 0x28 SF!=OF less
	case m_setg: // 0x26  SF=OF & ZF=0  Greater
	case m_setle: // 0x29 SF!=OF | ZF=1 Less or Equal
		oLeftNode = oNode1;
		oRightNode = oBuilder->NewMult(oNode2, oBuilder->NewConstant(-1));
		switch (opcode) {
		case m_setz: eOperator = OPERATOR_EQ; break;
		case m_setnz: eOperator = OPERATOR_NEQ; break;
		case m_setae: eOperator = OPERATOR_UGE; break; // 0x22  !C Above or Equal
		case m_setb:  eOperator = OPERATOR_ULT; break; // 0x23  C  below
		case m_seta:  eOperator = OPERATOR_UGT; break;// 0x24   !C & !Z  Above
		case m_setbe: eOperator = OPERATOR_ULE; break;// 0x25  C | Z  Below or Equal
		case m_setge: eOperator = OPERATOR_GE; break; // 0x27 SF=OF Greater or Equal
		case m_setl:  eOperator = OPERATOR_LT; break; // 0x28 SF!=OF less
		case m_setg:  eOperator = OPERATOR_GT; break; // 0x26  SF=OF & ZF=0  Greater
		case m_setle: eOperator = OPERATOR_LE; break; // 0x29 SF!=OF | ZF=1 Less or Equal
		}
		break;
	case m_sets:
	case m_seto:
		oLeftNode = oBuilder->NewAdd(oNode1, oNode2);
		oRightNode = oBuilder->NewConstant(0);
		switch (opcode) {
		case m_sets: eOperator = OPERATOR_LT; break;
		case m_seto: eOperator = OPERATOR_NEQ; break;
		}
		break;

	}

	return Condition::create(
		oLeftNode,
		eOperator,
		oRightNode
	);
}

Condition flag_mop_t::ConditionalInstructionShift(CodeBroker& oBuilder, mcode_t opcode)
{
	operator_t eOperator;
	DFGNode oLeftNode, oRightNode, oCarryNode;
	switch (opcode) {
	case m_setz:
	case m_setnz:
	case m_sets:
		oLeftNode = oBuilder->NewShift(oNode1, oNode2);
		oRightNode = oBuilder->NewConstant(0);
		switch (opcode) {
		case m_setz: eOperator = OPERATOR_EQ; break;
		case m_setnz: eOperator = OPERATOR_NEQ; break;
		case m_sets: eOperator = OPERATOR_LT; break;
		}
		break;
	case m_setae: // 0x22  !C Above or Equal
	case m_setb:  // 0x23  C  below
		oBuilder->NewShift(oNode1, oNode2, &oLeftNode);
		oRightNode = oBuilder->NewConstant(0);
		switch (opcode) {
		case m_setae: eOperator = OPERATOR_NEQ; break;
		case m_setb: eOperator = OPERATOR_EQ; break;
		}
		break;
	case m_seta: // 0x24   !C & !Z  Above
	case m_setbe: // 0x25  C | Z  Below or Equal

		m_seta,
			m_setbe,
			// TODO:后续看能不能根据cf寄存器将c设为与arm中类似
			/* transform (!C & !Z) to (!C | !Z) != 0 */
			oLeftNode = oBuilder->NewShift(oNode1, oNode2, &oCarryNode);
		oLeftNode = oBuilder->NewOr(
			oBuilder->NewXor(oLeftNode, oBuilder->NewConstant(0xffffffff)), // ~Z
			oCarryNode // C
		);
		oRightNode = oBuilder->NewConstant(0);
		switch (opcode) {
		case m_seta: eOperator = OPERATOR_NEQ; break;
		case m_setbe: eOperator = OPERATOR_EQ; break;
		}
		break;
	}

	return Condition::create(
		oLeftNode,
		eOperator,
		oRightNode
	);

}


Condition flag_mop_t::ConditionalInstructionBitwise(CodeBroker& oBuilder, mcode_t opcode)
{
	operator_t eOperator;
	DFGNode oLeftNode, oRightNode;

	if (eOperation == FLAG_MOP_BITWISE_XOR && (opcode == m_setz || opcode == m_setnz)) {
		return Condition::create(oNode1, opcode == m_setz ? OPERATOR_EQ : OPERATOR_NEQ, oNode2);
	}

	switch (eOperation) {
	case FLAG_MOP_MULT: oLeftNode = oBuilder->NewMult(oNode1, oNode2); break;
	case FLAG_MOP_BITWISE_AND: oLeftNode = oBuilder->NewAnd(oNode1, oNode2); break;
	case FLAG_MOP_BITWISE_OR: oLeftNode = oBuilder->NewOr(oNode1, oNode2); break;
	case FLAG_MOP_BITWISE_XOR: oLeftNode = oBuilder->NewXor(oNode1, oNode2); break;
	}
	oRightNode = oBuilder->NewConstant(0);

	switch (opcode) {
	case m_setz: eOperator = OPERATOR_EQ; break;
	case m_setnz: eOperator = OPERATOR_NEQ; break;
	case m_sets: eOperator = OPERATOR_LT; break;
	}
	return Condition::create(
		oLeftNode,
		eOperator,
		oRightNode
	);
}

rfc_ptr<flag_mop_t> flag_mop_t::Migrate(DFGraph& oGraph)
{
	rfc_ptr<flag_mop_t> lpFork(rfc_ptr<flag_mop_t>::create(*this));
	lpFork->oNode1 = oGraph->FindNode(oNode1->dwNodeId);
	lpFork->oNode2 = oGraph->FindNode(oNode2->dwNodeId);
	return lpFork;
}

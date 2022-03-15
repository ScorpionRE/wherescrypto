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

// TODO: 没有必要初始化aRegisters, 大小如何决定？???
void MicrocodeImpl::initialize(CodeBroker& oBuilder) {
	unsigned int i;
	aRegisters.reserve(16);

	for (i = 0; i < 16; i++) {
		aRegisters.push_back(oBuilder->NewRegister(i));
	}

	dwMaxCallDepth = oBuilder->MaxCallDepth();
}

// microcode应该没有PC寄存器
// TODO: 某些寄存器如eax映射mreg_t为8-11如何处理？？？
DFGNode MicrocodeImpl::GetRegister(CodeBroker& oBuilder, unsigned long lpInstructionAddress, mreg_t bReg) {
	wc_debug("mreg number: %d\n", bReg);
	return aRegisters[bReg];
}

processor_status_t MicrocodeImpl::SetRegister(CodeBroker& oBuilder, unsigned long lpInstructionAddress, unsigned long* lpNextAddress, mreg_t bReg, DFGNode& oNode)
{
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

// microcode没有必要？？？
DFGNode MicrocodeImpl::GetOperand(CodeBroker& oBuilder, const mop_t& stOperand, unsigned long lpInstructionAddress, bool bSetFlags)
{

	return DFGNode();
}

processor_status_t MicrocodeImpl::JumpToNode(CodeBroker& oBuilder, unsigned long* lpNextAddress, unsigned long lpInstructionAddress, DFGNode oAddress)
{
	unsigned long lpTarget(0);


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

// 一条一条分析microcode指令
processor_status_t MicrocodeImpl::instruction(CodeBroker& oBuilder, unsigned long* lpNextAddress, unsigned long lpAddress) {

	unsigned int i;
	unsigned int dwRegisterNo;
	int dwInstructionSize;
	// TODO: 得到指令操作码
	GenMicrocode(lpAddress);

	minsn_t mInstruction = hx_mba_t_for_all_topinsns;  // microcode instruction

	
	*lpNextAddress = lpAddress + dwInstructionSize;

	DFGNode oConditionNode;
	graph_process_t eVerdict = GRAPH_PROCESS_CONTINUE;
	Condition oCondition;
	switch (mInstruction.opcode) { // mcode_t op操作码

	case m_setz:  // 0x21 Z Equal 
	case m_setnz: // 0x20 !Z Not equal
		if (oZeroFlag != nullptr) {
			oCondition = oZeroFlag->ConditionalInstruction(oBuilder, mInstruction.opcode);
		}
		else {
		_missing_flags:
			wc_debug("[-] conditional instruction but flags are not set @ 0x%x\n", lpAddress);
			return PROCESSOR_STATUS_INTERNAL_ERROR;
		}
		break;


	case m_setae: // 0x22  !C Above or Equal
	case m_setb:  // 0x23  C  below
		if (oCarryFlag == nullptr) {
			oCondition = oCarryFlag->ConditionalInstruction(oBuilder, mInstruction.opcode);
		}
		else {
			goto _missing_flags;
		}
		break;


	case m_sets: // 0x1D   N  Negative
		if (oNegativeFlag != nullptr) {
			oCondition = oNegativeFlag->ConditionalInstruction(oBuilder, mInstruction.opcode);
		}
		else {
			goto _missing_flags;
		}
		break;


	case m_seto: // 0x1E   O  Overflow
		if (oOverflowFlag != nullptr) {
			oCondition = oOverflowFlag->ConditionalInstruction(oBuilder, mInstruction.opcode);
		}
		else {
			goto _missing_flags;
		}
		break;
	case m_setp: // 0x1F PF unordered/parity  TODO??? 
	{
		break;
	}
	case m_seta: // 0x24   !C & !Z  Above
	case m_setbe: // 0x25  C | Z  Below or Equal
		if (oCarryFlag == nullptr && oZeroFlag != nullptr) {
			if (oCarryFlag == oZeroFlag) {
			_flags_from_different_operations:
				wc_debug("[-] flags used in conditional instruction originate from two different operations which is not supported @ 0x%x\n", lpAddress);
				return PROCESSOR_STATUS_INTERNAL_ERROR;
			}
			oCondition = oCarryFlag->ConditionalInstruction(oBuilder, mInstruction.opcode);  //为什么这样做？？？
		}
		else {
			goto _missing_flags;
		}
		break;

	case m_setge: // 0x27 SF=OF Greater or Equal
	case m_setl:  // 0x28 SF!=OF less
		if (oNegativeFlag != nullptr && oOverflowFlag != nullptr) {
			if (oNegativeFlag != oOverflowFlag) {
				goto _flags_from_different_operations;
			}
			oCondition = oNegativeFlag->ConditionalInstruction(oBuilder, mInstruction.opcode);
		}
		else {
			goto _missing_flags;
		}
		break;

	case m_setg: // 0x26  SF=OF & ZF=0  Greater
	case m_setle: // 0x29 SF!=OF | ZF=1 Less or Equal
		if (oZeroFlag != nullptr && oNegativeFlag != nullptr && oOverflowFlag != nullptr) {
			if (oZeroFlag != oNegativeFlag || oZeroFlag != oOverflowFlag) {
				goto _flags_from_different_operations;
			}
			oCondition = oZeroFlag->ConditionalInstruction(oBuilder, mInstruction.opcode);
		}
		else {
			goto _missing_flags;
		}
		break;

	case m_ldx: {  
		/*
		* ldx  l=sel,r=off, d     // load register from memory  
		* 1. register
		* 2. TODO: 
		*/
		// processor_status_t eStatus;
		// for_all_topinsns();
		DFGNode oLoad;
		DFGNode oReg = GetRegister(oBuilder, lpAddress, mInstruction.l.r);

		break;

	}

	case m_stx: {
		DFGNode oData = GetRegister(oBuilder, lpAddress, mInstruction.l.r);  // 
		break;
	}


	
	case m_fmul:
	case m_mul: { // 0x0E mul l,r,d  l*r->d
		DFGNode oNode1, oNode2;
		oNode1 = GetOperand(oBuilder, mInstruction.l, lpAddress, false);
		oNode2 = GetOperand(oBuilder, mInstruction.r, lpAddress, false);
		DFGNode oMult = oBuilder->NewMult(oNode1, oNode2);
		processor_status_t eStatus = SetRegister(oBuilder, lpAddress, lpNextAddress, mInstruction.d.r,oMult);
		if (eStatus != PROCESSOR_STATUS_OK) {
			return eStatus;
		}
		// TODO: auxpref & aux_cond

	}
	case m_add:
	case m_fadd: {
		DFGNode oNode1, oNode2;
		oNode1 = GetOperand(oBuilder, mInstruction.l, lpAddress, false);
		oNode2 = GetOperand(oBuilder, mInstruction.r, lpAddress, false);
		DFGNode oAdd = oBuilder->NewAdd(oNode1, oNode2);
		processor_status_t eStatus = SetRegister(oBuilder, lpAddress, lpNextAddress, mInstruction.d.r, oAdd);
		if (eStatus != PROCESSOR_STATUS_OK) {
			return eStatus;
		}
		break;

	}
	case m_sub: 
	case m_fsub: {
		DFGNode oNode1, oNode2;
		oNode1 = GetOperand(oBuilder, mInstruction.l, lpAddress, false);
		oNode2 = GetOperand(oBuilder, mInstruction.r, lpAddress, false);
		oNode2 = oBuilder->NewMult(oNode2, oBuilder->NewConstant(-1));
		DFGNode oAdd = oBuilder->NewAdd(oNode1, oNode2);
		processor_status_t eStatus = SetRegister(oBuilder, lpAddress, lpNextAddress, mInstruction.d.r, oAdd);
		if (eStatus != PROCESSOR_STATUS_OK) {
			return eStatus;
		}
		break;

	}
	case m_cfadd: {
		DFGNode oNode1, oNode2;
		oNode1 = GetOperand(oBuilder, mInstruction.l, lpAddress, false);
		oNode2 = GetOperand(oBuilder, mInstruction.r, lpAddress, false);

		if (oCarryFlag != nullptr) {
			wc_debug("[-] instruction uses carry but carry not set @ 0x%x\n", lpAddress);
			return PROCESSOR_STATUS_INTERNAL_ERROR;
		}
		DFGNode oCarryNode = oCarryFlag->Carry(oBuilder);
		if (oCarryNode == nullptr) { //为nullpter???
			wc_debug("[-] instruction uses carry but unable to construct a value for it @ 0x%x\n", lpAddress);
			return PROCESSOR_STATUS_INTERNAL_ERROR;
		}
		oNode2 = oBuilder->NewAdd(oNode2, oCarryNode);
		break;
	}

	case m_ofadd: {
		break;
	}
	case m_cfshl: {
		break;
	}
	case m_fdiv:
	case m_udiv: {
		break;
	}
	case m_sdiv: {
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
		DFGNode oNode = GetOperand(oBuilder, mInstruction.l, lpAddress, false);  //TODO: 立即数到register  (stInstruction.auxpref & aux_cond
		processor_status_t eStatus = SetRegister(oBuilder, lpAddress, lpNextAddress,mInstruction.d.r  , oNode);
		if (eStatus != PROCESSOR_STATUS_OK) {
			return eStatus;
		}
		break;
	}
	case m_fneg:
	case m_neg: {
		DFGNode oNode = oBuilder->NewMult(GetOperand(oBuilder, mInstruction.l, lpAddress, false), oBuilder->NewConstant(-1));
		processor_status_t eStatus = SetRegister(oBuilder, lpAddress, lpNextAddress, mInstruction.d.r, oNode);
		if (eStatus != PROCESSOR_STATUS_OK) {
			return eStatus;
		}
		break;
	}
	case m_lnot: {
		break;
	}
	case m_bnot: {
		break;
	}



	case m_push: {
		ea_t dwSpec;
		bool bPost;
		bool bIncrement;
		bool bWriteback;
		int dwIncrement = 0;
		break;

	}
	case m_pop: {
		break;
	}
	case m_and: {
		DFGNode oSource;
		DFGNode oNode1, oNode2;
		oNode1 = GetRegister(oBuilder, lpAddress, mInstruction.l.r);
		// oNode2 = GetOperand(oBuilder, lpAddress, mInstruction.r);  //GetReigister???
		flag_mop_type_t eFlagOp;
		oSource = oBuilder->NewAnd(oNode1, oNode2); 
		eFlagOp = FLAG_MOP_BITWISE_AND; 
		break;
	}
	case m_xor: {
		DFGNode oSource;
		DFGNode oNode1, oNode2;
		oNode1 = GetRegister(oBuilder, lpAddress, mInstruction.l.r);
		// oNode2 = GetOperand(oBuilder, lpAddress, mInstruction.r);  //GetReigister???
		flag_mop_type_t eFlagOp;
		oSource = oBuilder->NewXor(oNode1, oNode2); 
		eFlagOp = FLAG_MOP_BITWISE_XOR; 
		break;
	}

	case m_or: {
		DFGNode oSource;
		DFGNode oNode1, oNode2;
		oNode1 = GetRegister(oBuilder, lpAddress, mInstruction.l.r);
		// oNode2 = GetOperand(oBuilder, lpAddress, mInstruction.r);  //GetReigister???
		flag_mop_type_t eFlagOp;
		oSource = oBuilder->NewOr(oNode1, oNode2); 
		eFlagOp = FLAG_MOP_BITWISE_OR; 
		break;
	}

	case m_shl: {
		DFGNode oNode1, oNode2;
		oNode1 = GetRegister(oBuilder, lpAddress, mInstruction.l.r);
		oNode2 = GetRegister(oBuilder, lpAddress, mInstruction.r.r);
		processor_status_t eStatus;
		DFGNode oSource;
		oSource = oBuilder->NewShift(oNode1, oNode2);


		SetFlag(FLAG_MOP_SHIFT, oNode1, oNode2);
		eStatus = SetRegister(oBuilder, lpAddress, lpNextAddress, dwRegisterNo, oSource);
		if (eStatus != PROCESSOR_STATUS_OK) {
			return eStatus;
		}
		break;
	}
	case m_shr: {
		//TODO: 右移

	}
	case m_sar: {
		DFGNode oNode1, oNode2;
		oNode1 = GetRegister(oBuilder, lpAddress, mInstruction.l.r);
		oNode2 = GetRegister(oBuilder, lpAddress, mInstruction.r.r);
		processor_status_t eStatus;
		DFGNode oSource;
		oSource = oBuilder->NewRotate(oNode1, oNode2);


		SetFlag(FLAG_MOP_SHIFT, oNode1, oNode2);
		eStatus = SetRegister(oBuilder, lpAddress, lpNextAddress, dwRegisterNo, oSource);
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
		DFGNode oAddress = GetOperand(oBuilder, mInstruction.l, lpAddress, false);
		return JumpToNode(oBuilder, lpNextAddress, lpAddress, oAddress);
	}
	case m_call: {
		DFGNode oAddress = GetOperand(oBuilder, mInstruction.l, lpAddress, false);
		// TODO: instruction size在microcode中
		SetRegister(oBuilder, lpAddress, lpNextAddress,mInstruction.d.r , oBuilder->NewConstant(lpAddress + dwInstructionSize));
		PushCallStack(lpAddress + dwInstructionSize);
		return JumpToNode(oBuilder, lpNextAddress, lpAddress, oAddress);

	}
	case m_icall: {

	}
	case m_ijmp: {

	}
	case m_ret: { //TODO: call 保存的返回值在哪
		DFGNode oAddress = GetRegister(oBuilder, lpAddress, 14);
		return JumpToNode(oBuilder, lpNextAddress, lpAddress, oAddress);
		break;
	}
	case m_jz:
	case m_jnz: {
		dwRegisterNo = mInstruction.l.r;
		DFGNode oNode = GetRegister(oBuilder, lpAddress, dwRegisterNo);
		Condition oCondition(Condition::create(oNode, mInstruction.opcode == m_jz ? OPERATOR_EQ : OPERATOR_NEQ, oBuilder->NewConstant(0)));
		eVerdict = oBuilder->IntroduceCondition(oCondition, lpAddress + dwInstructionSize);
		if (eVerdict == GRAPH_PROCESS_SKIP) {
			goto _skip;
		}
		else if (eVerdict == GRAPH_PROCESS_INTERNAL_ERROR) {
			return PROCESSOR_STATUS_INTERNAL_ERROR;
		}
		break;
	}

	case m_jcnd: {
		break;
	}

	case m_jae: {
		break;
	}
	case m_jb: {
		break;
	}
	case m_ja: {
		break;
	}
	case m_jbe: {
		break;
	}
	case m_jg: {
		break;
	}
	case m_jge: {
		break;
	}
	case m_jl: {
		break;
	}
	case m_jle: {
		break;
	}
	case m_jtbl: {

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
		wc_debug("unhandled instruction type: %d at 0x%x", mInstruction.opcode, lpAddress);
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
	if (hexdsp != nullptr)
		term_hexrays_plugin();

	// generate microcode
	hexrays_failure_t hf;
	mba_ranges_t mbr = mba_ranges_t();
	func_t* fn = get_func(get_screen_ea());
	if (fn == NULL)
	{
		warning("Please position the cursor within a function");
		return true;
	}

	ea_t ea1 = fn->start_ea;
	ea_t ea2 = fn->end_ea;
	hf = hexrays_failure_t();

	flags_t F = get_flags(ea1);
	if (!is_code(F))
	{
		warning("The selected range must start with an instruction");
		return true;
	}
	mbr.ranges.push_back(range_t(ea1, ea2));
	mba_t* mba = gen_microcode(mbr, &hf, NULL, DECOMP_WARNINGS);
	if (mba == NULL)
	{
		warning("%a: %s", hf.errea, hf.desc().c_str());
		return true;
	}

	msg("Successfully generated microcode for %a..%a\n", ea1, ea2);
	// vd_printer_t vp;
	// mba->print(vp);

	// basic blocks

	int qty = mba->qty;
	mblock_t* blocks = mba->blocks->nextb;
	msg("%d basic blocks", qty);


	// instructions 
	msg("No %d basic block", blocks->serial);
	minsn_t* ins = blocks->head;

	msg("instructs opcode:%x, l:%d.%d  , r:%d.%d , d: %d.%d ", ins->opcode, ins->l.r, ins->l.size, ins->r.r, ins->r.size, ins->d.r, ins->d.size);




	// We must explicitly delete the microcode array
	delete mba;
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

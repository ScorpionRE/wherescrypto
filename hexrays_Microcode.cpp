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


void MicrocodeImpl::initialize(CodeBroker& oBuilder) {}

//register
DFGNode MicrocodeImpl::GetRegister(CodeBroker& oBuilder, unsigned long lpInstructionAddress, unsigned char bReg) {
	
	return aRegisters[bReg];
}

processor_status_t MicrocodeImpl::SetRegister(CodeBroker& oBuilder, unsigned long lpInstructionAddress, unsigned long* lpNextAddress, unsigned char bReg, DFGNode& oNode)
{
	return processor_status_t();
}

DFGNode MicrocodeImpl::GetOperandShift(CodeBroker& oBuilder, DFGNode& oBaseNode, DFGNode& oShift, char bShiftType, bool bSetFlags)
{

	return DFGNode();
}

DFGNode MicrocodeImpl::GetOperand(CodeBroker& oBuilder, const mop_t& stOperand, unsigned long lpInstructionAddress, bool bSetFlags)
{
	return DFGNode();
}

processor_status_t MicrocodeImpl::JumpToNode(CodeBroker& oBuilder, unsigned long* lpNextAddress, unsigned long lpInstructionAddress, DFGNode oAddress)
{
	return processor_status_t();
}

void MicrocodeImpl::PushCallStack(unsigned long lpAddress)
{
}

void MicrocodeImpl::PopCallStack(unsigned long lpAddress)
{
}

// 一条一条分析microcode指令
processor_status_t MicrocodeImpl::instruction(CodeBroker& oBuilder, unsigned long* lpNextAddress, unsigned long lpAddress) {
	insn_t stInstruction;
	unsigned int i;
	unsigned int dwRegisterNo;
	int dwInstructionSize;
	// TODO: 得到指令操作码
	API_LOCK();
	dwInstructionSize = decode_insn(&stInstruction, lpAddress);
	API_UNLOCK();

	minsn_t mInstruction = hx_mba_t_for_all_topinsns;  // microcode instruction

	if (dwInstructionSize <= 0) {
		return PROCESSOR_STATUS_INTERNAL_ERROR;
	}

	*lpNextAddress = lpAddress + dwInstructionSize;

	DFGNode oConditionNode;
	// graph_process_t eVerdict = GRAPH_PROCESS_CONTINUE;
	Condition oCondition;
	switch (mInstruction.opcode) { // mcode_t op操作码

	case m_setz:  // 0x21 Z Equal 
	case m_setnz: // 0x20 !Z Not equal
		if (oZeroFlag != nullptr) {
			oCondition = oZeroFlag->ConditionalInstruction(oBuilder, stInstruction.segpref);
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
			oCondition = oCarryFlag->ConditionalInstruction(oBuilder, stInstruction.segpref);
		}
		else {
			goto _missing_flags;
		}
		break;


	case m_sets: // 0x1D   N  Negative
		if (oNegativeFlag != nullptr) {
			oCondition = oNegativeFlag->ConditionalInstruction(oBuilder, stInstruction.segpref);
		}
		else {
			goto _missing_flags;
		}
		break;


	case m_seto: // 0x1E   O  Overflow
		if (oOverflowFlag != nullptr) {
			oCondition = oOverflowFlag->ConditionalInstruction(oBuilder, stInstruction.segpref);
		}
		else {
			goto _missing_flags;
		}
		break;
	case m_setp: // 0x1F PF unordered/parity  TODO???


	case m_seta: // 0x24   !C & !Z  Above
	case m_setbe: // 0x25  C | Z  Below or Equal
		if (oCarryFlag == nullptr && oZeroFlag != nullptr) {
			if (oCarryFlag == oZeroFlag) {
			_flags_from_different_operations:
				wc_debug("[-] flags used in conditional instruction originate from two different operations which is not supported @ 0x%x\n", lpAddress);
				return PROCESSOR_STATUS_INTERNAL_ERROR;
			}
			oCondition = oCarryFlag->ConditionalInstruction(oBuilder, stInstruction.segpref);  //为什么这样做？？？
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
			oCondition = oNegativeFlag->ConditionalInstruction(oBuilder, stInstruction.segpref);
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
			oCondition = oZeroFlag->ConditionalInstruction(oBuilder, stInstruction.segpref);
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
		DFGNode oReg = GetRegister(oBuilder, lpAddress, stInstruction.ops[1].reg);



	}

	case m_stx: {
		DFGNode oData = GetRegister(oBuilder, lpAddress, stInstruction.ops[0].reg);  // 
	}
			  
	case m_mul: { // 0x0E mul l,r,d  l*r->d
		DFGNode oNode1, oNode2;
		oNode1 = GetOperand(oBuilder, mInstruction.l, lpAddress, false);
		oNode2 = GetOperand(oBuilder, mInstruction.r, lpAddress, false);
		DFGNode oMult = oBuilder->NewMult(oNode1, oNode2);
		processor_status_t eStatus = SetRegister(oBuilder, lpAddress, lpNextAddress, mInstruction.d);
		if (eStatus != PROCESSOR_STATUS_OK) {
			return eStatus;
		}
		// TODO: auxpref & aux_cond

	}
	case m_add: {
		DFGNode oNode1, oNode2;
		oNode1 = GetOperand(oBuilder, mInstruction.l, lpAddress, false);
		oNode2 = GetOperand(oBuilder, mInstruction.r, lpAddress, false);
		DFGNode oAdd = oBuilder->NewAdd(oNode1, oNode2);
		processor_status_t eStatus = SetRegister(oBuilder, lpAddress, lpNextAddress, mInstruction.d, oAdd);
		if (eStatus != PROCESSOR_STATUS_OK) {
			return eStatus;
		}
		break;

	}
	case m_sub: {
		DFGNode oNode1, oNode2;
		oNode1 = GetOperand(oBuilder, mInstruction.l, lpAddress, false);
		oNode2 = GetOperand(oBuilder, mInstruction.r, lpAddress, false);
		oNode2 = oBuilder->NewMult(oNode2, oBuilder->NewConstant(-1));
		DFGNode oAdd = oBuilder->NewAdd(oNode1, oNode2);
		processor_status_t eStatus = SetRegister(oBuilder, lpAddress, lpNextAddress, mInstruction.d, oAdd);
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

	case m_mov: {
		DFGNode oNode = GetOperand(oBuilder, mInstruction.l, lpAddress, false);  //TODO: 立即数到register  (stInstruction.auxpref & aux_cond
		processor_status_t eStatus = SetRegister(oBuilder, lpAddress, lpNextAddress,mInstruction.d  , oNode);
		if (eStatus != PROCESSOR_STATUS_OK) {
			return eStatus;
		}
		break;
	}

	case m_neg: {
		DFGNode oNode = oBuilder->NewMult(GetOperand(oBuilder, mInstruction.l, lpAddress, false), oBuilder->NewConstant(-1));
		processor_status_t eStatus = SetRegister(oBuilder, lpAddress, lpNextAddress, mInstruction.d, oNode);
		if (eStatus != PROCESSOR_STATUS_OK) {
			return eStatus;
		}
		break;
	}

	case m_ldc: {

	}

	case m_push: {
		ea_t dwSpec;
		bool bPost;
		bool bIncrement;
		bool bWriteback;
		int dwIncrement = 0;


	}
	case m_pop: {

	}
	case m_and: {
		DFGNode oSource;
		DFGNode oNode1, oNode2;
		oNode1 = GetRegister(oBuilder, lpAddress, mInstruction.l);
		// oNode2 = GetOperand(oBuilder, lpAddress, mInstruction.r);  //GetReigister???
		flag_mop_type_t eFlagOp;
		oSource = oBuilder->NewAnd(oNode1, oNode2); 
		eFlagOp = FLAG_MOP_BITWISE_AND; 
		break;
	}
	case m_xor: {
		DFGNode oSource;
		DFGNode oNode1, oNode2;
		oNode1 = GetRegister(oBuilder, lpAddress, mInstruction.l);
		// oNode2 = GetOperand(oBuilder, lpAddress, mInstruction.r);  //GetReigister???
		flag_mop_type_t eFlagOp;
		oSource = oBuilder->NewXor(oNode1, oNode2); 
		eFlagOp = FLAG_MOP_BITWISE_XOR; 
		break;
	}

	case m_or: {
		DFGNode oSource;
		DFGNode oNode1, oNode2;
		oNode1 = GetRegister(oBuilder, lpAddress, mInstruction.l);
		// oNode2 = GetOperand(oBuilder, lpAddress, mInstruction.r);  //GetReigister???
		flag_mop_type_t eFlagOp;
		oSource = oBuilder->NewOr(oNode1, oNode2); 
		eFlagOp = FLAG_MOP_BITWISE_OR; 
		break;
	}

	case m_shl: {
		DFGNode oNode1, oNode2;
		oNode2 = 
	}
	case m_shr: {


	}
	case m_sar: {

	}
	}



	return PROCESSOR_STATUS_OK;
}
 
bool MicrocodeImpl::ShouldClean(DFGNode& oNode) {
	return false;
}


bool MicrocodeImpl::genMicrocode() {
	if (hexdsp != nullptr)
		term_hexrays_plugin();

	ea_t ea1, ea2;
	if (!read_range_selection(NULL, &ea1, &ea2))
	{
		warning("Please select a range of addresses to analyze");
		return true;
	}

	flags_t F = get_flags(ea1);
	if (!is_code(F))
	{
		warning("The selected range must start with an instruction");
		return true;
	}

	// generate microcode
	hexrays_failure_t hf;
	mba_ranges_t mbr;
	mbr.ranges.push_back(range_t(ea1, ea2));
	mba_t* mba = gen_microcode(mbr, &hf, NULL, DECOMP_WARNINGS);
	if (mba == NULL)
	{
		warning("%a: %s", hf.errea, hf.desc().c_str());
		return true;
	}

	wc_debug("Successfully generated microcode for %a..%a\n", ea1, ea2);
	vd_printer_t vp;
	mba->print(vp);

	// We must explicitly delete the microcode array
	delete mba;
	return true;

}

Processor MicrocodeImpl::Migrate(DFGraph oGraph) {
	genMicrocode();
	return Processor::typecast(Microcode::create());
}

Condition flag_mop_t::ConditionalInstruction(CodeBroker& oBuilder, char segpref)
{
	return Condition();
}

Condition flag_mop_t::ConditionalInstructionAdd(CodeBroker& oBuilder, char segpref)
{
	return Condition();
}

Condition flag_mop_t::ConditionalInstructionShift(CodeBroker& oBuilder, char segpref)
{
	return Condition();
}

Condition flag_mop_t::ConditionalInstructionBitwise(CodeBroker& oBuilder, char segpref)
{
	return Condition();
}

rfc_ptr<flag_mop_t> flag_mop_t::Migrate(DFGraph& oGraph)
{
	return rfc_ptr<flag_mop_t>();
}

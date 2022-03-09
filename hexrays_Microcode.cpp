#include <ida.hpp>
#include <ua.hpp>
#include <idp.hpp>
#include <allins.hpp>
#include <segregs.hpp>


#include "common.hpp"
#include "hexrays_Microcode.hpp"
#include "DFGraph.hpp"
#include "Broker.hpp"


void MicrocodeImpl::initialize(CodeBroker& oBuilder) {}

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

	if (dwInstructionSize <= 0) {
		return PROCESSOR_STATUS_INTERNAL_ERROR;
	}

	*lpNextAddress = lpAddress + dwInstructionSize;

	DFGNode oConditionNode;
	graph_process_t eVerdict = GRAPH_PROCESS_CONTINUE;
	Condition oCondition;
	switch (stInstruction.segpref) { // mcode_t op操作码

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

		case 
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

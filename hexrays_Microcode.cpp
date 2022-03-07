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

processor_status_t MicrocodeImpl::instruction(CodeBroker& oBuilder, unsigned long* lpNextAddress, unsigned long lpAddress) {
	insn_t stInstruction;
	unsigned int i;
	unsigned int dwRegisterNo;
	int dwInstructionSize;

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
	switch (stInstruction.segpref) {

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

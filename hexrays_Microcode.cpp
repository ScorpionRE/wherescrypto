#include <ida.hpp>
#include <ua.hpp>
#include <idp.hpp>
#include <allins.hpp>
#include <segregs.hpp>
#include <hexrays.hpp>

#include "common.hpp"
#include "hexrays_Microcode.hpp"
#include "DFGraph.hpp"
#include "Broker.hpp"

void MicrocodeImpl::initialize(CodeBroker& oBuilder) {}

processor_status_t MicrocodeImpl::instruction(CodeBroker& oBuilder, unsigned long* lpNextAddress, unsigned long lpAddress) {
	return PROCESSOR_STATUS_OK;
}
 
bool MicrocodeImpl::ShouldClean(DFGNode& oNode) {
	return false;
}

Processor MicrocodeImpl::Migrate(DFGraph oGraph) {
	return Processor::create();
}

#pragma once

#include <vector>
#include <list>
#include <mutex>
#include <hexrays.hpp>

#include "types.hpp"
#include "Processor.hpp"




class MicrocodeImpl : public ProcessorImpl {
public:
	inline MicrocodeImpl() { }
	inline ~MicrocodeImpl() { }

	void initialize(CodeBroker& oBuilder);
	
	processor_status_t instruction(CodeBroker& oBuilder, unsigned long* lpNextAddress, unsigned long lpAddress);
	bool ShouldClean(DFGNode& oNode);

	bool genMicrocode();

protected:
	virtual Processor Migrate(DFGraph oGraph);

	friend class CodeBrokerImpl;
};


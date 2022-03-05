#pragma once

#include <vector>
#include <list>
#include <mutex>

#include "types.hpp"
#include "Processor.hpp"

class MicrocodeImpl : public ProcessorImpl {
public:
	inline MicrocodeImpl() { }
	inline ~MicrocodeImpl() { }

	void initialize(CodeBroker& oBuilder);
	//bool genMicrocode();
	processor_status_t instruction(CodeBroker& oBuilder, unsigned long* lpNextAddress, unsigned long lpAddress);
	bool ShouldClean(DFGNode& oNode);

protected:
	virtual Processor Migrate(DFGraph oGraph);

	friend class CodeBrokerImpl;
};


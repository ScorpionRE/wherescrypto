#pragma once

#include <vector>
#include <list>
#include <mutex>
#include <hexrays.hpp>

#include "types.hpp"
#include "Processor.hpp"


typedef enum {
	FLAG_MOP_UNSET = 0,
	FLAG_MOP_ADD,
	FLAG_MOP_SHIFT,
	FLAG_MOP_MULT,
	FLAG_MOP_BITWISE_AND,
	FLAG_MOP_BITWISE_OR,
	FLAG_MOP_BITWISE_XOR
} flag_mop_type_t;

class flag_mop_t : public virtual ReferenceCounted {
public:
	inline flag_mop_t() : eOperation(FLAG_MOP_UNSET) { }
	inline flag_mop_t(const flag_mop_t&) = default;
	inline flag_mop_t(DFGNode& oNode1, DFGNode& oNode2, flag_mop_type_t eOperation)
		: oNode1(oNode1), oNode2(oNode2), eOperation(eOperation) { }

	DFGNode Carry(CodeBroker& oBuilder);
	Condition ConditionalInstruction(CodeBroker& oBuilder, char segpref);
	Condition ConditionalInstructionAdd(CodeBroker& oBuilder, char segpref);
	Condition ConditionalInstructionShift(CodeBroker& oBuilder, char segpref);
	Condition ConditionalInstructionBitwise(CodeBroker& oBuilder, char segpref);

	rfc_ptr<flag_mop_t> Migrate(DFGraph& oGraph);

	DFGNode oNode1;
	DFGNode oNode2;
	flag_mop_type_t eOperation;
};

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

private:
	rfc_ptr<flag_mop_t> oCarryFlag;
	rfc_ptr<flag_mop_t> oOverflowFlag;
	rfc_ptr<flag_mop_t> oZeroFlag;
	rfc_ptr<flag_mop_t> oNegativeFlag;

	inline void SetFlag(flag_mop_type_t eOperation, DFGNode oNode1, DFGNode oNode2) {
		rfc_ptr<flag_mop_t> oFlagObj(rfc_ptr<flag_mop_t>::create(oNode1, oNode2, eOperation));

		switch (eOperation) {
		case FLAG_MOP_ADD:
			oCarryFlag = oFlagObj;
			oOverflowFlag = oFlagObj;
			oZeroFlag = oFlagObj;
			oNegativeFlag = oFlagObj;
			break;
		case FLAG_MOP_SHIFT:
			oCarryFlag = oFlagObj;
			oZeroFlag = oFlagObj;
			oNegativeFlag = oFlagObj;
			break;
		case FLAG_MOP_MULT:
		case FLAG_MOP_BITWISE_AND:
		case FLAG_MOP_BITWISE_OR:
		case FLAG_MOP_BITWISE_XOR:
			oZeroFlag = oFlagObj;
			oNegativeFlag = oFlagObj;
			break;
		}
	}
	DFGNode GetRegister(CodeBroker& oBuilder, unsigned long lpInstructionAddress, unsigned char bReg);
	processor_status_t SetRegister(CodeBroker& oBuilder, unsigned long lpInstructionAddress, unsigned long* lpNextAddress, unsigned char bReg, DFGNode& oNode);
	DFGNode GetOperandShift(CodeBroker& oBuilder, DFGNode& oBaseNode, DFGNode& oShift, char bShiftType, bool bSetFlags);
	DFGNode GetOperand(CodeBroker& oBuilder, const op_t& stOperand, unsigned long lpInstructionAddress, bool bSetFlags = false);
	processor_status_t JumpToNode(CodeBroker& oBuilder, unsigned long* lpNextAddress, unsigned long lpInstructionAddress, DFGNode oAddress);
	void PushCallStack(unsigned long lpAddress);
	void PopCallStack(unsigned long lpAddress);

	friend class CodeBrokerImpl;
};


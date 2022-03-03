#pragma once
#include <vector>
#include <list>
#include <mutex>

#include "types.hpp"
#include "Processor.hpp"

typedef enum {
	FLAG_OP_UNSET = 0,
	FLAG_OP_ADD,
	FLAG_OP_SHIFT,
	FLAG_OP_MULT,
	FLAG_OP_BITWISE_AND,
	FLAG_OP_BITWISE_OR,
	FLAG_OP_BITWISE_XOR
} flag_op_type_t;

class flag_op_t : public virtual  ReferenceCounted {
public:

};


class MicrocodeImpl : public ProcessorImpl
{
public:
	inline MicrocodeImpl() { }
	inline ~MicrocodeImpl() { }
	MicrocodeImpl(const MicrocodeImpl&) = default;

	std::vector<DFGNode> aRegisters;
	std::list<unsigned long> aCallStack;

	int dwMaxCallDepth;

	void initialize(CodeBroker& oBuilder);
	bool genMicrocode();
	processor_status_t instruction(CodeBroker& oBuilder, unsigned long* lpNextAddress, unsigned long lpAddress);
	bool ShouldClean(DFGNode& oNode);

protected:
	virtual Processor Migrate(DFGraph oGraph);

private:
	rfc_ptr<flag_op_t> oCarryFlag;
	rfc_ptr<flag_op_t> oOverflowFlag;
	rfc_ptr<flag_op_t> oZeroFlag;
	rfc_ptr<flag_op_t> oNegativeFlag;

	inline void SetFlag(flag_op_type_t eOperation, DFGNode oNode1, DFGNode oNode2) {
		rfc_ptr<flag_op_t> oFlagObj(rfc_ptr<flag_op_t>::create(oNode1, oNode2, eOperation));

		switch (eOperation) {
		case FLAG_OP_ADD:
			oCarryFlag = oFlagObj;
			oOverflowFlag = oFlagObj;
			oZeroFlag = oFlagObj;
			oNegativeFlag = oFlagObj;
			break;
		case FLAG_OP_SHIFT:
			oCarryFlag = oFlagObj;
			oZeroFlag = oFlagObj;
			oNegativeFlag = oFlagObj;
			break;
		case FLAG_OP_MULT:
		case FLAG_OP_BITWISE_AND:
		case FLAG_OP_BITWISE_OR:
		case FLAG_OP_BITWISE_XOR:
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


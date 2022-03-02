#include <ida.hpp>
#include <ua.hpp>
#include <idp.hpp>
#include <allins.hpp>
#include <segregs.hpp>

#include "common.hpp"
#include "Microcode.hpp"
#include "DFGraph.hpp"
#include "Broker.hpp"
#include "Condition.hpp"


typedef enum {
	LSL,          // logical left         LSL #0 - don't shift
	LSR,          // logical right        LSR #0 means LSR #32
	ASR,          // arithmetic right     ASR #0 means ASR #32
	ROR,          // rotate right         ROR #0 means RRX
	RRX,          // extended rotate right
} shift_t;

typedef enum {
	cEQ,          // 0000 Z                        Equal
	cNE,          // 0001 !Z                       Not equal
	cCS,          // 0010 C                        Unsigned higher or same
	cCC,          // 0011 !C                       Unsigned lower
	cMI,          // 0100 N                        Negative
	cPL,          // 0101 !N                       Positive or Zero
	cVS,          // 0110 V                        Overflow
	cVC,          // 0111 !V                       No overflow
	cHI,          // 1000 C & !Z                   Unsigned higher
	cLS,          // 1001 !C | Z                   Unsigned lower or same
	cGE,          // 1010 (N & V) | (!N & !V)      Greater or equal
	cLT,          // 1011 (N & !V) | (!N & V)      Less than
	cGT,          // 1100 !Z & ((N & V)|(!N & !V)) Greater than
	cLE,          // 1101 Z | (N & !V) | (!N & V)  Less than or equal
	cAL,          // 1110 Always
	cNV          // 1111 Never
} cond_t;

void MicrocodeImpl::initialize(CodeBroker& oBuilder) {
	unsigned int i;
	aRegisters.reserve(16);


}

processor_status_t MicrocodeImpl::JumpToNode(CodeBroker& oBuilder, unsigned long* lpNextAddress, unsigned long lpInstructionAddress, DFGNode oAddress) {

}


//gen_microcode
processor_status_t MicrocodeImpl::instruction(CodeBroker& oBuilder, unsigned long* lpNextAddress, unsigned long lpAddress) {
	
}



#include "includes.h"
using namespace std;



enum Status {
	S_SUCCESS,
	S_INVALID_DATA,
	S_FAILED,
	S_ERROR
}


int16_t programCounter = 0;
std::vector<size_t> instructionSet;
std::array<int8_t, 32> registers;



namespace helper {

void readInstructionsFromFile(const std::string& filePath) {
	instructionSet.push_back(0x000000);
}


inline int8_t* handleOperand(int8_t fromInstr, bool isImmediate) {
	if (isImmediate) {
		return &fromInstr;
	}

	size_t index = fromInstr & 0x1F;
	return &(registers[index]);
}


inline int16_t combineRegisters(size_t A, size_t B) {
	return (registers[A] << 8) | (registers[B]) & 0xFFFF;
}

}


namespace operators {

static int8_t NOP(int8_t* A, int8_t* B, size_t controlBits, Status* status) {
	return 0;
}


static int8_t SET(int8_t* A, int8_t* B, size_t controlBits, Status* status) {
	if (controlBits & 0b0010) {
		*status = S_INVALID_DATA;
	} else {
		(*A) = (*B);
	}
	return 0;
}


static int8_t IO_(int8_t* A, int8_t* B, size_t controlBits, Status* status) {
	return 0; //Implement later.
}


static int8_t ADD(int8_t* A, int8_t* B, size_t controlBits, Status* status) {
	return (*A) + (*B);
}


static int8_t SUB(int8_t* A, int8_t* B, size_t controlBits, Status* status) {
	return (*A) - (*B);
}


static int8_t MUL(int8_t* A, int8_t* B, size_t controlBits, Status* status) {
	return (*A) * (*B);
}


static int8_t DIV(int8_t* A, int8_t* B, size_t controlBits, Status* status) {
	if (B == 0) {
		*status = S_ERROR;
		return 0;
	}
	return (*A) / (*B);
}


static int8_t MOD(int8_t* A, int8_t* B, size_t controlBits, Status* status) {
	if (B == 0) {
		*status = S_ERROR;
		return 0;
	}
	return (*A) % (*B);
}


static int8_t EQU(int8_t* A, int8_t* B, size_t controlBits, Status* status) {
	if (controlBits & 0b1000) {
		//Invert value.
		return (*A) != (*B);
	} else {
		return (*A) == (*B);
	}
}


static int8_t CMP(int8_t* A, int8_t* B, size_t controlBits, Status* status) {
	switch(controlBits & 0b1100) {
		case 0b0000: { //Greater-than;
			return (*A) > (*B);
		}
		case 0b1000: { //Less-than;
			return (*A) < (*B);
		}
		case 0b0100: { //Greater-than or equal to;
			return (*A) >= (*B);
		}
		case 0b1100: { //Less-than or equal to;
			return (*A) <= (*B);
		}
	}
}


static int8_t BRN(int8_t* A, int8_t* B, size_t controlBits, Status* status) {
	bool cond = (controlBits&0b1000) ? !(*A) : *A; //Bit 0 changes BRN-EQU to BRN-NEQ.
	if (cond || (controlBits&0b0100)) { //JMP uses bit 2.
		programCounter = helper::combineRegisters(29, 30);
	}
	return 0;
}

}


std::array<std::pair<std::function<int8_t(int8_t* A, int8_t* B, size_t controlBits, Status* status)>, bool>, 16> opc = {
	{operators::NOP, false},
	{operators::SET, false},
	{operators::IO_, true},
	{operators::ADD, true},
	{operators::SUB, true},
	{operators::MUL, true},
	{operators::DIV, true},
	{operators::MOD, true},
	{operators::EQU, true},
	{operators::CMP, true}
	
	{operators::NOP, false},
	{operators::NOP, false},
	{operators::NOP, false},
	{operators::NOP, false},
	{operators::NOP, false},
	{operators::NOP, false},
}


void handleInstruction(size_t instruction, int8_t* result, bool* hasResult, Status* status) {
	size_t controlBits = (instruction & 0xF00000) >> 20;
	bool operandAIsImmediate = (controlBits & 0b0010);
	bool operandBIsImmediate = (controlBits & 0b0001);

	size_t operatorIndex = (instruction & 0x0F0000) >> 16;
	int8_t* operandA = helper::handleOperand((instruction & 0x00FF00) >> 8, operandAIsImmediate);
	int8_t* operandB = helper::handleOperand((instruction & 0x0000FF), operandBIsImmediate);

	std::pair funcData = opc[operatorIndex];
	int8_t opRes = funcData.first(operandA, operandB, controlBits, status);
	*hasResult = funcData.second;
	if (*hasResult) {
		*result = opRes;
	}
}





std::unordered_map<Status, std::string> reasonMap = {
	{S_SUCCESS, "The instruction executed successfully."},
	{S_INVALID_DATA, "The data passed was invalid for the instruction."},
	{S_FAILED, "The instruction failed to execute."},
	{S_ERROR, "An error occurred while executing instruction."},
};

std::array<std::string, 16> operatorNameMap = {
	"NOP", "SET", "IO_", "",
	"ADD", "SUB", "MUL", "DIV",
	"", "", "", "",
	"", "", "", "",
};

void showError(Status status, size_t instruction) {
	std::cout << "Instruction " << programCounter << " [" << instruction << "] has failed!" << std::endl;

	std::string reason = reasonMap[status];
	std::cout << "Reason: " << reason << std::endl;

	size_t operatorIndex = (instruction & 0x0F0000) >> 16;
	std::string operatorName = operatorNameMap[operatorIndex];
	std::cout << "Attempted operation: " << operatorName;
	size_t controlBits = (instruction & 0xF00000) >> 20;
	bool operandAIsImmediate = (controlBits & 0b0010);
	if (operandAIsImmediate) {std::cout << ", Operand A was immediate"}
	bool operandBIsImmediate = (controlBits & 0b0001);
	if (operandBIsImmediate) {std::cout << ", Operand B was immediate"}
}

int main() {
	std::string filePath = "cfab/testNew.cfab";
	helper::readInstructionsFromFile(filePath);
	int16_t result;
	bool hasResult;

	while (programCounter < instructionSet.size()) {
		Status status = S_SUCCESS;
		size_t instruction = instructionSet[programCounter];
		handleInstruction(instruction, &result, &hasResult, &status);
		if (status != S_SUCCESS) {
			showError(status, instruction);
		} else if (hasResult) {
			registers[15] = result;
		}

		programCounter++;
	}
}
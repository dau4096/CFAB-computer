#ifndef TESTS_H
#define TESTS_H

#include "includes.h"
#include "constants.h"
#include "utils.h"
#include "instr.h"
using namespace std;



std::vector<unsigned int> testInstructions;

namespace tests {







//////// Memory management ////////
void testSETImmediate() {
	//A = B
	testInstructions = {
		0x410001u, //SET r0 to #1
	};
	CFAB::runInstructionSet(testInstructions);
	assert((
		"Register 0 was not set to value 1",
		registers[0u] == 1
	));
}
void testSETRegister() {
	//A = B
	testInstructions = {
		0x410005u, //SET r0 to #5
		0x010100u, //SET r1 to r0's value
	};
	CFAB::runInstructionSet(testInstructions);
	assert((
		"Register 1 was not set to the value in Register 0",
		registers[1u] == registers[0u]
	));
}


void testMOV() {
	//A ~ B
	testInstructions = {
		0x410003u, //SET r0 to #3
		0x020001u //MOV r0 to r1
	};
	CFAB::runInstructionSet(testInstructions);
	assert((
		"Register 1 was not set to 1",
		registers[1u] == 3
	));
	assert((
		"Register 0 was not reset to 0",
		registers[0u] == 0
	));
}


void testClear() {
	//Clears all registers to 0.
	testInstructions = {
		0x410102u, //SET r1 to #2
		0x9D0000u, //EXT --> CLEAR_REGISTERS to 0.
	};
	CFAB::runInstructionSet(testInstructions);
	assert((
		"Registers were not cleared to 0",
		registers[1u] == 0
	));
}
//////// Memory management ////////







//////// Maths ////////
void testADD() {
	//A + B
	testInstructions = {
		0x410001u, //SET r0 to #1
		0x410102u, //SET r1 to #2
		0x030001u //ADD r0 to r1
	};
	CFAB::runInstructionSet(testInstructions);
	assert((
		"Addition result was not 3 [1+2]",
		registers[REG_RESULT] == 3
	));
}

void testSUB() {
	//A - B
	testInstructions = {
		0x41000Au, //SET r0 to #10
		0x410102u, //SET r1 to #2
		0x040001u //SUB r1 from r0
	};
	CFAB::runInstructionSet(testInstructions);
	assert((
		"Subtraction result was not 8 [10-2]",
		registers[REG_RESULT] == 8
	));
}

void testMUL() {
	//A * B
	testInstructions = {
		0x410008u, //SET r0 to #8
		0x410104u, //SET r1 to #4
		0x050001u //MUL r0 by r1
	};
	CFAB::runInstructionSet(testInstructions);
	assert((
		"Multiplication result was not 32 [8*4]",
		registers[REG_RESULT] == 32
	));
}

void testDIV() {
	//A / B
	testInstructions = {
		0x410008u, //SET r0 to #8
		0x410104u, //SET r1 to #4
		0x060001u //DIV r0 by r1
	};
	CFAB::runInstructionSet(testInstructions);
	assert((
		"Division result was not 2 [8/4]",
		registers[REG_RESULT] == 2
	));
}

void testINV() {
	//-A
	testInstructions = {
		0x410008u, //SET r0 to #8
		0x270000u //Numerically invert r0
	};
	CFAB::runInstructionSet(testInstructions);
	assert((
		"Numerical inversion result was not -8 [-(8)]",
		registers[REG_RESULT] == -8
	));
}

void testABS() {
	//abs(A)
	testInstructions = {
		0x4100F8u, //SET r0 to #-8
		0x370000u //Absolute value of r0
	};
	CFAB::runInstructionSet(testInstructions);
	assert((
		"Numerical inversion result was not 8 [abs(-8)]",
		registers[REG_RESULT] == 8
	));	
}

void testSHF_L() {
	//A << B
	testInstructions = {
		0x410002u, //SET r0 to #2
		0x4C0004u, //Left-shift r0 by #4
	};
	CFAB::runInstructionSet(testInstructions);
	assert((
		"Left-shift result was not 32 [2 << 4]",
		registers[REG_RESULT] == 32
	));
}

void testSHF_R() {
	//A >> B
	testInstructions = {
		0x410020u, //SET r0 to #32
		0x5C0004u, //Right-shift r0 by #4
	};
	CFAB::runInstructionSet(testInstructions);
	assert((
		"Right-shift result was not 2 [32 >> 4]",
		registers[REG_RESULT] == 2
	));
}
//////// Maths ////////







//////// Logic ////////
void testAND() {
	//Bitwise AND
	testInstructions = {
		0x41001Fu, //SET r0 to 31
		0x410107u, //SET r1 to 7
		0x150001u //Bitwise r0 AND r1
	};
	CFAB::runInstructionSet(testInstructions);
	assert((
		"Bitwise AND result was not 7 [31 & 7]",
		registers[REG_RESULT] == 7
	));
}

void testOR() {
	//Bitwise OR
	testInstructions = {
		0x410009u, //SET r0 to 9
		0x410105u, //SET r1 to 5
		0x130001u //Bitwise r0 OR r1
	};
	CFAB::runInstructionSet(testInstructions);
	assert((
		"Bitwise OR result was not 13 [9 | 5]",
		registers[REG_RESULT] == 13
	));
}

void testEQU() {
	//A == B and A != B
	testInstructions = {
		0x410009u, //SET r0 to 9
		0x410108u, //SET r1 to 8

		0x080001u, //r0 == r1 [FALSE]
		0x027F02u, //Move result to r2

		0x180001u, //r0 != r1 [TRUE]
		0x027F03u //Move result to r3
	};

	CFAB::runInstructionSet(testInstructions);

	assert((
		"Equals result was not 0 [9 == 8]",
		registers[2u] == 0
	));
	assert((
		"Not-Equals result was not 1 [9 != 8]",
		registers[3u] == 1
	));
}

void testXOR() {
	//A ^ B and ~(A ^ B)
	testInstructions = {
		0x410009u, //SET r0 to 9
		0x410108u, //SET r1 to 8

		0x280001u, //r0 ^ r1
		0x027F02u, //Move result to r2

		0x380001u, //~(r0 ^ r1)
		0x027F03u //Move result to r3
	};

	CFAB::runInstructionSet(testInstructions);

	assert((
		"Equals result was not 1 [9 ^ 8]",
		registers[2u] == 1
	));
	assert((
		"Not-Equals result was not -2 [~(9 ^ 8)]",
		registers[3u] == -2
	));	
}
//////// Logic ////////







//////// Other ////////
void testBRN() {
	//Conditional branch, Jump, inverse conditional branch

	//Conditional
	programCounter = 0u;
	testInstructions = {
		0x417F01u, //SET rOP [r127] to 1
		0xEA0008u //Branch to line 8 if rOP
	};
	CFAB::runInstructionSet(testInstructions);
	assert((
		"Did not jump to line 8 when result was true",
		programCounter == 8u
	));	

	//Unconditional
	programCounter = 0u;
	testInstructions = {
		0x417F00u, //SET rOP [r127] to 0
		0xCA0010u //Branch to line 16 unconditionally
	};
	CFAB::runInstructionSet(testInstructions);
	assert((
		"Did not jump to line 16 unconditionally",
		programCounter == 16u
	));	

	//Inverse conditional
	programCounter = 0u;
	testInstructions = {
		0x417F00u, //SET rOP [r127] to 0
		0xFA0020u //Branch to line 32 if not result
	};
	CFAB::runInstructionSet(testInstructions);
	assert((
		"Did not jump to line 32 when result is 0",
		programCounter == 32u
	));	

}
//////// Other ////////







const std::vector<std::function<void()>> tests = {
	//Memory
	testSETImmediate, testSETRegister, testMOV, testClear,

	//Maths
	testADD, testSUB, testMUL, testDIV,
	testINV, testABS,
	testSHF_L, testSHF_R,

	//Logic
	testAND, testOR, testEQU, testXOR,

	//Other
	testBRN
};


void run() {
	//Tests specific cases using assert.

	//Main tests
	for (std::function<void()> test : tests) {
		std::cout << std::endl;
		std::fill(registers.begin(), registers.end(), static_cast<int8_t>(0)); //Clear all registers.
		programCounter = 0u;
		test();
		std::cout << "\033[1;32m[TEST PASSED]\033[0;m" << std::endl;
	}
	std::cout << std::endl;

}

}


#endif

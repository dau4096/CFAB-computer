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
	registers[0u] = 5; //SET r0 to 5
	testInstructions = {
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
	registers[0u] = 3; //SET r0 to 3
	testInstructions = {
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
	registers[1u] = 2; //SET r1 to 2
	testInstructions = {
		0x9D0000u, //EXT --> CLEAR_REGISTERS to 0.
	};
	CFAB::runInstructionSet(testInstructions);
	assert((
		"Registers were not cleared to 0",
		registers[1u] == 0
	));
}


void testRAMwrite() {
	//Write to RAM.
	registers[REG_RESULT] = 16; //SET result register to 16
	testInstructions = {
		0xED0004, //Write value in result register to RAM address 4
	};
	CFAB::runInstructionSet(testInstructions);
	assert((
		"Value in result register was not written to RAM address 4",
		randomAccessMemory[4u] == registers[REG_RESULT]
	));
}


void testRAMread() {
	//Read value from RAM
	randomAccessMemory[12u] = -8; //Force-Write value to RAM to test
	testInstructions = {
		0xFD000Cu, //Read value at RAM address 12 into result register
	};
	CFAB::runInstructionSet(testInstructions);
	assert((
		"Value in RAM address 12 was not written to result register",
		registers[REG_RESULT] == randomAccessMemory[12u]
	));
}

//////// Memory management ////////







//////// Maths ////////

void testADD() {
	//A + B
	registers[0u] = 1; //SET r0 to 1
	registers[1u] = 2; //SET r1 to 2
	testInstructions = {
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
	registers[0u] = 10; //SET r0 to 10
	registers[1u] = 2; //SET r1 to 2
	testInstructions = {
		0x040001u, //SUB r1 from r0
	};
	CFAB::runInstructionSet(testInstructions);
	assert((
		"Subtraction result was not 8 [10-2]",
		registers[REG_RESULT] == 8
	));
}


void testMUL() {
	//A * B
	registers[0u] = 8; //SET r0 to 8
	registers[1u] = 4; //SET r1 to 4
	testInstructions = {
		0x050001u, //MUL r0 by r1
	};
	CFAB::runInstructionSet(testInstructions);
	assert((
		"Multiplication result was not 32 [8*4]",
		registers[REG_RESULT] == 32
	));
}


void testDIV() {
	//A / B
	registers[0u] = 8; //SET r0 to 8
	registers[1u] = 4; //SET r1 to 4
	testInstructions = {
		0x060001u, //DIV r0 by r1
	};
	CFAB::runInstructionSet(testInstructions);
	assert((
		"Division result was not 2 [8/4]",
		registers[REG_RESULT] == 2
	));
}


void testINV() {
	//-A
	registers[0u] = 8; //SET r0 to 8
	testInstructions = {
		0x270000u, //Numerically invert r0
	};
	CFAB::runInstructionSet(testInstructions);
	assert((
		"Numerical inversion result was not -8 [-(8)]",
		registers[REG_RESULT] == -8
	));
}


void testABS() {
	//abs(A)
	registers[0u] = -8; //SET r0 to -8
	testInstructions = {
		0x370000u, //Absolute value of r0
	};
	CFAB::runInstructionSet(testInstructions);
	assert((
		"Numerical inversion result was not 8 [abs(-8)]",
		registers[REG_RESULT] == 8
	));	
}


void testSHF_L() {
	//A << B
	registers[0u] = 2; //SET r0 to 2
	testInstructions = {
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
	registers[0u] = 32; //SET r0 to 32
	testInstructions = {
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
	registers[0u] = 31; //SET r0 to 31
	registers[1u] = 7; //SET r1 to 7
	testInstructions = {
		0x150001u, //Bitwise r0 AND r1
	};
	CFAB::runInstructionSet(testInstructions);
	assert((
		"Bitwise AND result was not 7 [31 & 7]",
		registers[REG_RESULT] == 7
	));
}


void testOR() {
	//Bitwise OR
	registers[0u] = 9; //SET r0 to 9
	registers[1u] = 5; //SET r1 to 5
	testInstructions = {
		0x130001u, //Bitwise r0 OR r1
	};
	CFAB::runInstructionSet(testInstructions);
	assert((
		"Bitwise OR result was not 13 [9 | 5]",
		registers[REG_RESULT] == 13
	));
}


void testEQU() {
	//A == B and A != B
	registers[0u] = 9; //SET r0 to 9
	registers[1u] = 8; //SET r1 to 8
	testInstructions = {
		0x080001u, //r0 == r1 [FALSE]
		0x027F02u, //Move result to r2

		0x180001u, //r0 != r1 [TRUE]
		0x027F03u, //Move result to r3
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
	registers[0u] = 9; //SET r0 to 9
	registers[1u] = 8; //SET r1 to 8
	testInstructions = {
		0x280001u, //r0 ^ r1
		0x027F02u, //Move result to r2

		0x380001u, //~(r0 ^ r1)
		0x027F03u, //Move result to r3
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
	registers[REG_RESULT] = 1; //SET result register to 1
	testInstructions = {
		0xEA0008u //Branch to line 8 if rOP
	};
	CFAB::runInstructionSet(testInstructions);
	assert((
		"Did not jump to line 8 when result was true",
		programCounter == 8u
	));	

	//Unconditional
	programCounter = 0u;
	registers[REG_RESULT] = 0; //SET result register to 0
	testInstructions = {
		0xCA0010u //Branch to line 16 unconditionally
	};
	CFAB::runInstructionSet(testInstructions);
	assert((
		"Did not jump to line 16 unconditionally",
		programCounter == 16u
	));	

	//Inverse conditional
	programCounter = 0u;
	registers[REG_RESULT] = 0; //SET result register to 0
	testInstructions = {
		0xFA0020u //Branch to line 32 if not result
	};
	CFAB::runInstructionSet(testInstructions);
	assert((
		"Did not jump to line 32 when result is 0",
		programCounter == 32u
	));	

}


void testExit() {
	//Halt instruction
	run = true;
	testInstructions = {
		0x0D0000u, //EXT --> HALT_PROGRAM
	};
	CFAB::runInstructionSet(testInstructions);
	assert((
		"Program did not halt",
		run == false
	));	
}


void testInput() {
	//Input value
	inputBits = 0x01FFu;
	testInstructions = {
		0x0B0001, //Read input bits to registers 0 and 1
	};
	CFAB::runInstructionSet(testInstructions);
	assert((
		"Registers 0 and 1 did not contain the correct 16 input bits",
		(registers[0u] == 1) && (registers[1u] == -1)
	));	
}


void testOutput() {
	//Output value
	registers[0u] = 1;  //0x01 Rep.
	registers[1u] = -1; //0xFF Rep.
	//Total is 0x01FF
	testInstructions = {
		0x1B0001, //Write output bits from values stored in registers 0 and 1
	};
	CFAB::runInstructionSet(testInstructions);
	assert((
		"Values stored in r0 and r1 were not written to output",
		outputBits == 0x01FFu
	));	
}

//////// Other ////////







const std::vector<std::function<void()>> tests = {
	//Memory
	testSETImmediate, testSETRegister, testMOV, testClear,
	testRAMwrite, testRAMread,

	//Maths
	testADD, testSUB, testMUL, testDIV,
	testINV, testABS,
	testSHF_L, testSHF_R,

	//Logic
	testAND, testOR, testEQU, testXOR,

	//Other
	testBRN, testExit, testInput, testOutput
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

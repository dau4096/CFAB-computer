#ifndef TESTS_H
#define TESTS_H

#include "includes.h"
#include "constants.h"
#include "utils.h"
#include "instr.h"
using namespace std;



std::vector<Instruction> testInstructions;
unsigned int numPassed, numRan;

inline void assertOrThrow(bool condition, const std::string& message) {
	numRan++;
    if (!condition) {
        throw std::runtime_error(message);
    } else {
    	numPassed++;
    }
}


namespace tests {







//////// Memory management ////////

void testSETImmediate() {
	//A = B
	std::cout << "SET-immediate ";
	testInstructions = {
		Instruction(0x410001u), //SET r0 to #1
	};
	CFAB::runInstructionSet(testInstructions);
	assertOrThrow(
		registers[0u] == 1,
		"Register 0 was not set to value 1"
	);
}


void testSETRegister() {
	//A = B
	std::cout << "SET-copy ";
	registers[0u] = 5; //SET r0 to 5
	testInstructions = {
		Instruction(0x010100u), //SET r1 to r0's value
	};
	CFAB::runInstructionSet(testInstructions);
	assertOrThrow(
		registers[1u] == registers[0u],
		"Register 1 was not set to the value in Register 0"
	);
}


void testMOV() {
	//A ~ B
	std::cout << "MOV ";
	registers[0u] = 3; //SET r0 to 3
	testInstructions = {
		Instruction(0x020001u) //MOV r0 to r1
	};
	CFAB::runInstructionSet(testInstructions);
	assertOrThrow(
		registers[1u] == 3,
		"Register 1 was not set to 1"
	);
	assertOrThrow(
		registers[0u] == 0,
		"Register 0 was not reset to 0"
	);
}


void testRegisterClear() {
	//Clears all registers to 0.
	std::cout << "CLEAR-registers ";
	registers[1u] = 2; //SET r1 to 2
	testInstructions = {
		Instruction(0x9D0000u), //EXT --> CLEAR_REGISTERS to 0.
	};
	CFAB::runInstructionSet(testInstructions);
	assertOrThrow(
		registers[1u] == 0,
		"Registers were not cleared to 0"
	);
}


void testRAMwrite() {
	//Write to RAM.
	std::cout << "RAM-write ";
	registers[REG_RESULT] = 16; //SET result register to 16
	testInstructions = {
		Instruction(0xED0004), //Write value in result register to RAM address 4
		Instruction(0xED0E12) //Write value in result register to screen address 18.
	};
	CFAB::runInstructionSet(testInstructions);
	assertOrThrow(
		randomAccessMemory[4u] == registers[REG_RESULT],
		"Value in result register was not written to RAM address 4"
	);
	assertOrThrow(
		randomAccessMemory[0xE12u] == registers[REG_RESULT],
		"Value in result register was not written to screen address 18"
	);

	//Maybe test read/write from inval addrs?
}


void testRAMread() {
	//Read value from RAM
	std::cout << "RAM-read ";
	randomAccessMemory[12u] = static_cast<uint8_t>(-8); //Force-Write value to RAM to test (RAM is uint8_t, so value must be converted.)
	testInstructions = {
		Instruction(0xFD000Cu), //Read value at RAM address 12 into result register
	};
	CFAB::runInstructionSet(testInstructions);
	assertOrThrow(
		registers[REG_RESULT] == static_cast<int8_t>(randomAccessMemory[12u]),
		"Value in RAM address 12 was not written to result register"
	);
}


void testROMCopy() {
	//Test copying from ROM into RAM
	std::cout << "ROMcopy ";

	readOnlyMemory = { //Add test data to the ROM
		'C', 'F', 'A', 'B', 'R', 'O', 'M', 1u
	};
	readOnlyMemoryIndices.push_back({0u, 4u});
	readOnlyMemoryIndices.push_back({4u, 7u});

	testInstructions = {
		//Load rom segment 0 into RAM starting at RAM[12]
		Instruction(0x413C00), //rX = 0;
		Instruction(0xEF000C), //LOAD #0 #12

		//Load rom segment 1 into RAM starting at RAM[32]
		Instruction(0x413C01), //rX = 1;
		Instruction(0xEF0020), //LOAD #0 #32
	};
	CFAB::runInstructionSet(testInstructions);

	bool ROM0success = true;
	for (unsigned int idx=0u; idx<4; idx++) {
		if (readOnlyMemory[idx] != randomAccessMemory[12u + idx]) {
			ROM0success = false;
			break;
		}
	}

	bool ROM1success = true;
	for (unsigned int idx=0u; idx<3; idx++) {
		if (readOnlyMemory[4 + idx] != randomAccessMemory[32u + idx]) {
			ROM1success = false;
			break;
		}
	}


	assertOrThrow(
		ROM0success, "ROM segment 0 was not correctly copied into RAM[12:]"
	);
	assertOrThrow(
		ROM1success, "ROM segment 1 was not correctly copied into RAM[32:]"
	);
}


void testRAMManagement() {
	//Test copying, clearing sections of RAM.
	std::cout << "RAMcopy/RAMclear ";

	const uint8_t testRAMdata[7] = {
		0xFF, 0x3F, 0x20, 0x78, 0x38, 0x69, 0x67,
	};

	//Place it in 2 parts of RAM (one for moving, one for clearing.)
	memcpy(
		randomAccessMemory + 396u,
		testRAMdata, 7 * sizeof(uint8_t)
	);
	memcpy(
		randomAccessMemory + 2048u,
		testRAMdata, 7 * sizeof(uint8_t)
	);

	testInstructions = {
		Instruction(0x413C7F), //rX = #x7F (clear value)
		Instruction(0x413D08), //rY = #x08 (End address hi-bits)
		Instruction(0x413E07), //rZ = #x07 (End address lo-bits)
		Instruction(0xCF0800), //RAMclear #x8 #x00

		Instruction(0x413C01), //rX = #x01 (End address hi-bits)
		Instruction(0x413D93), //rY = #x93 (End address lo-bits)
		Instruction(0x413E01), //rZ = #x01 (Dest address hi-bits)
		Instruction(0x413BA4), //rW = #xA4 (Dest address lo-bits)
		Instruction(0xDF018C)  //RAMcopy #x01 #x8C
	};
	CFAB::runInstructionSet(testInstructions);

	bool clearSuccess = true;
	for (unsigned int idx=0u; idx<7u; idx++) {
		if (randomAccessMemory[2048+idx] != 0x7Fu) { //Check cleared to fill value.
			clearSuccess = false;
			break;
		}
	}

	bool copySrcSuccess = true, copyDestSuccess = true;
	for (unsigned int idx=0u; idx<7u; idx++) {
		const uint8_t& expected = testRAMdata[idx];
		if (randomAccessMemory[396u+idx] != expected) {
			copySrcSuccess = false;
		} 
		if (randomAccessMemory[420u+idx] != expected) {
			copyDestSuccess = false;
		}
	}


	assertOrThrow(
		clearSuccess, "RAMclear did not fill RAM section correctly."
	);
	assertOrThrow(
		copySrcSuccess, "RAMcopy illegally modified source data."
	);
	assertOrThrow(
		copyDestSuccess, "RAMcopy did not fill destination with correct data."
	);
}

//////// Memory management ////////







//////// Maths ////////

void testADD() {
	//A + B
	std::cout << "ADD ";
	registers[0u] = 1; //SET r0 to 1
	registers[1u] = 2; //SET r1 to 2
	testInstructions = {
		Instruction(0x030001u) //ADD r0 to r1
	};
	CFAB::runInstructionSet(testInstructions);
	assertOrThrow(
		registers[REG_RESULT] == 3,
		"Addition result was not 3 [1+2]"
	);
}


void testSUB() {
	//A - B
	std::cout << "SUB ";
	registers[0u] = 10; //SET r0 to 10
	registers[1u] = 2; //SET r1 to 2
	testInstructions = {
		Instruction(0x040001u), //SUB r1 from r0
	};
	CFAB::runInstructionSet(testInstructions);
	assertOrThrow(
		registers[REG_RESULT] == 8,
		"Subtraction result was not 8 [10-2]"
	);
}


void testMUL() {
	//A * B
	std::cout << "MUL ";
	registers[0u] = 8; //SET r0 to 8
	registers[1u] = 4; //SET r1 to 4
	testInstructions = {
		Instruction(0x050001u), //MUL r0 by r1
	};
	CFAB::runInstructionSet(testInstructions);
	assertOrThrow(
		registers[REG_RESULT] == 32,
		"Multiplication result was not 32 [8*4]"
	);
}


void testDIV() {
	//A / B
	std::cout << "DIV ";
	registers[0u] = 8; //SET r0 to 8
	registers[1u] = 4; //SET r1 to 4
	testInstructions = {
		Instruction(0x060001u), //DIV r0 by r1
	};
	CFAB::runInstructionSet(testInstructions);
	assertOrThrow(
		registers[REG_RESULT] == 2,
		"Division result was not 2 [8/4]"
	);

	//r0 is still 8 at this point, try divide by zero.
	registers[1u] = 0; //SET r1 to 0
	testInstructions = {
		Instruction(0x060001u), //DIV r0 by r1
	};
	CFAB::runInstructionSet(testInstructions);
	assertOrThrow(
		registers[REG_RESULT] == 0,
		"Division-by-zero result was not 0 [8/0]"
	);
}


void testMOD() {
	//A / B
	std::cout << "MOD ";
	registers[0u] = 12; //SET r0 to 12
	registers[1u] = 7; //SET r1 to 7
	testInstructions = {
		Instruction(0x160001u), //MOD r0 by r1
	};
	CFAB::runInstructionSet(testInstructions);
	assertOrThrow(
		registers[REG_RESULT] == 5,
		"Modulo result was not 5 [12%7]"
	);
}


void testINV() {
	//-A
	std::cout << "INV ";
	registers[0u] = 8; //SET r0 to 8
	testInstructions = {
		Instruction(0x270000u), //Numerically invert r0
	};
	CFAB::runInstructionSet(testInstructions);
	assertOrThrow(
		registers[REG_RESULT] == -8,
		"Numerical inversion result was not -8 [-(8)]"
	);
}


void testABS() {
	//abs(A)
	std::cout << "ABS ";
	registers[0u] = -8; //SET r0 to -8
	testInstructions = {
		Instruction(0x370000u), //Absolute value of r0
	};
	CFAB::runInstructionSet(testInstructions);
	assertOrThrow(
		registers[REG_RESULT] == 8,
		"Numerical inversion result was not 8 [abs(-8)]"
	);
}


void testSHF_L() {
	//A << B
	std::cout << "SHF-left ";
	registers[0u] = 2; //SET r0 to 2
	testInstructions = {
		Instruction(0x4C0004u), //Left-shift r0 by #4
	};
	CFAB::runInstructionSet(testInstructions);
	assertOrThrow(
		registers[REG_RESULT] == 32,
		"Left-shift result was not 32 [2 << 4]"
	);
}


void testSHF_R() {
	//A >> B
	std::cout << "SHF-right ";
	registers[0u] = 32; //SET r0 to 32
	testInstructions = {
		Instruction(0x5C0004u), //Right-shift r0 by #4
	};
	CFAB::runInstructionSet(testInstructions);
	assertOrThrow(
		registers[REG_RESULT] == 2,
		"Right-shift result was not 2 [32 >> 4]"
	);
}


void testSequenceArithmetic() {
	//Doing a sequence of maths operations to check state handling.
	std::cout << "Arithmetic-Sequencing ";
	//Calculates the 9th fibonacci number.
	testInstructions = {
		Instruction(0x410000), //r0 = #0 ($iter)
		Instruction(0x410101), //r1 = #1 ($prev)
		Instruction(0x410201), //r2 = #1 ($current)
		//:loop
		Instruction(0x030102), //r1 + r2
		Instruction(0x020201), //r2 ~ r1
		Instruction(0x023F02), //rOP ~ r2

		Instruction(0x430001), //r0 + #1
		Instruction(0x023F00), //rOP ~ r0
		Instruction(0x590009), //r0 < #9
		Instruction(0xEA0003)  //BRN 3 (:loop)
	};
	CFAB::runInstructionSet(testInstructions);

	//Final "current" value is stored in r2, check that it matches the expected.
	assertOrThrow(
		registers[2u] == 89,
		"Sequenced Arithmetic did not preserve internal state correctly. (Result was incorrect)"
	);
}

//////// Maths ////////







//////// Logic ////////

void testAND() {
	//Bitwise AND
	std::cout << "AND ";
	registers[0u] = 31; //SET r0 to 31
	registers[1u] = 7; //SET r1 to 7
	testInstructions = {
		Instruction(0x150001u), //Bitwise r0 AND r1
	};
	CFAB::runInstructionSet(testInstructions);
	assertOrThrow(
		registers[REG_RESULT] == 7,
		"Bitwise AND result was not 7 [31 & 7]"
	);
}


void testOR() {
	//Bitwise OR
	std::cout << "OR ";
	registers[0u] = 9; //SET r0 to 9
	registers[1u] = 5; //SET r1 to 5
	testInstructions = {
		Instruction(0x130001u), //Bitwise r0 OR r1
	};
	CFAB::runInstructionSet(testInstructions);
	assertOrThrow(
		registers[REG_RESULT] == 13,
		"Bitwise OR result was not 13 [9 | 5]"
	);
}


void testEQU() {
	//A == B and A != B
	std::cout << "EQU/NEQ ";
	registers[0u] = 9; //SET r0 to 9
	registers[1u] = 8; //SET r1 to 8
	testInstructions = {
		Instruction(0x080001u), //r0 == r1 [FALSE]
		Instruction(0x023F02u), //Move result to r2

		Instruction(0x180001u), //r0 != r1 [TRUE]
		Instruction(0x023F03u), //Move result to r3
	};

	CFAB::runInstructionSet(testInstructions);

	assertOrThrow(
		registers[2u] == 0,
		"Equals result was not 0 [9 == 8]"
	);
	assertOrThrow(
		registers[3u] == 1,
		"Not-Equals result was not 1 [9 != 8]"
	);
}


void testXOR() {
	//A ^ B and ~(A ^ B)
	std::cout << "XOR/XNOR ";
	registers[0u] = 9; //SET r0 to 9
	registers[1u] = 8; //SET r1 to 8
	testInstructions = {
		Instruction(0x280001u), //r0 ^ r1
		Instruction(0x023F02u), //Move result to r2

		Instruction(0x380001u), //~(r0 ^ r1)
		Instruction(0x023F03u), //Move result to r3
	};

	CFAB::runInstructionSet(testInstructions);

	assertOrThrow(
		registers[2u] == 1,
		"XOR result was not 1 [9 ^ 8]"
	);
	assertOrThrow(
		registers[3u] == -2,
		"XNOR result was not -2 [~(9 ^ 8)]"
	);	
}


void testComparisons() {
	//>, <, etc.
	std::cout << "GRT/GTE/LSS/LSE ";

	//When the 2 test values DO NOT EQUAL EACH OTHER.
	registers[0u] = -1;
	registers[1u] =  1;
	testInstructions = {
		Instruction(0x090001), //r0 > r1
		Instruction(0x023F02), //rOP ~ r2

		Instruction(0x190001), //r0 < r1
		Instruction(0x023F03), //rOP ~ r3

		Instruction(0x290001), //r0 >= r1
		Instruction(0x023F04), //rOP ~ r4

		Instruction(0x390001), //r0 <= r1
		Instruction(0x023F05)  //rOP ~ r5
	};
	CFAB::runInstructionSet(testInstructions);

	assertOrThrow(
		registers[2u] == 0,
		"GRT result was not 0 [-1 > 1]"
	);
	assertOrThrow(
		registers[3u] == 1,
		"LSS result was not 1 [-1 < 1]"
	);
	assertOrThrow(
		registers[4u] == 0,
		"GTE result was not 0 [-1 >= 1]"
	);
	assertOrThrow(
		registers[5u] == 1,
		"LSE result was not 1 [-1 <= 1]"
	);


	//When the 2 test values DO EQUAL EACH OTHER.
	registers[0u] = 8;
	registers[1u] = 8;
	for (uint8_t i=2u; i<=5u; i++) {registers[i] = 0u; /* Reset the answers from the previous tests */}
	testInstructions = {
		Instruction(0x090001), //r0 > r1
		Instruction(0x023F02), //rOP ~ r2

		Instruction(0x190001), //r0 < r1
		Instruction(0x023F03), //rOP ~ r3

		Instruction(0x290001), //r0 >= r1
		Instruction(0x023F04), //rOP ~ r4

		Instruction(0x390001), //r0 <= r1
		Instruction(0x023F05)  //rOP ~ r5
	};
	CFAB::runInstructionSet(testInstructions);

	assertOrThrow(
		registers[2u] == 0,
		"GRT result was not 0 [8 > 8]"
	);
	assertOrThrow(
		registers[3u] == 0,
		"LSS result was not 0 [8 < 8]"
	);
	assertOrThrow(
		registers[4u] == 1,
		"GTE result was not 1 [8 >= 8]"
	);
	assertOrThrow(
		registers[5u] == 1,
		"LSE result was not 1 [8 <= 8]"
	);
}


void testSequenceLogic() {
	//Doing a sequence of logic operations to check state handling.
	std::cout << "Logic-Sequencing ";
	//(!(A || B) && C) ^ D
	testInstructions = {
		Instruction(0x410001), //r0 = #1 ($A)
		Instruction(0x410100), //r1 = #0 ($B)
		Instruction(0x410201), //r2 = #1 ($C)
		Instruction(0x410301), //r3 = #1 ($D)

		Instruction(0x030001), //A || B (A + B)
		Instruction(0x073F00), //! rOP
		Instruction(0x053F02), //rOP && C (rOP * C)
		Instruction(0x183F03)  //rOP ^ D (rOP != D)
	};
	CFAB::runInstructionSet(testInstructions);

	//Result is stored in rOP, and should be 1.
	assertOrThrow(
		registers[REG_RESULT] == 1,
		"Sequenced Logic did not preserve internal state correctly. (Result was incorrect)"
	);
}

//////// Logic ////////







//////// Other ////////

void testBRN() {
	//Conditional branch, Jump, inverse conditional branch
	std::cout << "BEQ/JMP/BNE ";
	Instruction* endPTR;

	//Conditional
	registers[REG_RESULT] = 1; //SET result register to 1
	testInstructions = {
		Instruction(0xEA0008u) //Branch to line 8 if rOP
	};
	CFAB::runInstructionSet(testInstructions, &endPTR);
	assertOrThrow(
		endPTR == &(testInstructions[8]),
		"Did not jump to line 8 when result was true"
	);	

	//Unconditional
	registers[REG_RESULT] = 0; //SET result register to 0
	testInstructions = {
		Instruction(0xCA0010u) //Branch to line 16 unconditionally
	};
	CFAB::runInstructionSet(testInstructions, &endPTR);
	assertOrThrow(
		endPTR == &(testInstructions[16]),
		"Did not jump to line 16 unconditionally"
	);

	//Inverse conditional
	registers[REG_RESULT] = 0; //SET result register to 0
	testInstructions = {
		Instruction(0xFA0020u) //Branch to line 32 if not result
	};
	CFAB::runInstructionSet(testInstructions, &endPTR);
	assertOrThrow(
		endPTR == &(testInstructions[32]),
		"Did not jump to line 32 when result is 0"
	);

}


void testExit() {
	//Halt instruction
	std::cout << "HALT ";
	run = true;
	testInstructions = {
		Instruction(0x0D0000u), //EXT --> HALT_PROGRAM
	};
	CFAB::runInstructionSet(testInstructions);
	assertOrThrow(
		run == false,
		"Program did not halt "
	);
}


void testInput() {
	//Input value
	std::cout << "I_O-input ";
	inputBits = 0x01FFu;
	testInstructions = {
		Instruction(0x0B0001), //Read input bits to registers 0 and 1
	};
	CFAB::runInstructionSet(testInstructions);
	assertOrThrow(
		(registers[0u] == 1) && (registers[1u] == -1),
		"Registers 0 and 1 did not contain the correct 16 input bits"
	);	
}


void testOutput() {
	//Output value
	std::cout << "I_O-output ";
	registers[0u] = 1;  //0x01 Rep.
	registers[1u] = -1; //0xFF Rep.
	//Total is 0x01FF
	testInstructions = {
		Instruction(0x1B0001), //Write output bits from values stored in registers 0 and 1
	};
	CFAB::runInstructionSet(testInstructions);
	assertOrThrow(
		outputBits == 0x01FFu,
		"Values stored in r0 and r1 were not written to output"
	);
}


void testOverflows() {
	std::cout << "Overflow behaviour ";
	registers[0u] = 127; //Maximum positive value
	registers[1u] = -128; //Minimum negative value
	registers[2u] = 64; //A Test value

	testInstructions = {
		Instruction(0x030002), //r0 + r2
		Instruction(0x023F03), //rOP ~ r3

		Instruction(0x040102), //r1 - r2
		Instruction(0x023F04), //rOP ~ r4

		Instruction(0x450203), //r2 * #3
		Instruction(0x023F05), //rOP ~ r5

		Instruction(0x4502FD), //r2 * #-3
		Instruction(0x023F06), //rOP ~ r6

		Instruction(0x4C0202), //r2 << 2
		Instruction(0x023F07), //rOP ~ r7

		Instruction(0x5C0207), //r2 >> 7
		Instruction(0x023F08)  //rOP ~ r8
	};
	CFAB::runInstructionSet(testInstructions);

	assertOrThrow(
		registers[3u] == -65,
		"Addition overflow did not wrap correctly. [127 + 64 != -65]"
	);
	assertOrThrow(
		registers[4u] == 64,
		"Addition underflow did not wrap correctly. [-128 - 64 != 64]"
	);
	assertOrThrow(
		registers[5u] == -64,
		"Multiplication overflow did not wrap correctly. [64 * 3 != -64]"
	);
	assertOrThrow(
		registers[6u] == 64,
		"Multiplication underflow did not wrap correctly. [64 * -3 != 64]"
	);
	assertOrThrow(
		registers[7u] == 0,
		"Left-shift overflow was not 0. [64 << 2]"
	);
	assertOrThrow(
		registers[8u] == 0,
		"Right-shift underflow was not 0. [64 >> 7]"
	);
}


void testPRINTandCOUTs() {
	//Check that PRINT/COUT gives the correct response.
	//https://stackoverflow.com/a/12061076 ← Read std::cout
	std::cout << "PRINT/COUT ";

	//Set up ROM for PRINT.
	readOnlyMemory = {
		'C', 'F', 'A', 'B', ' ', 'i', 's', ' ', 'n', 'e', 'a', 't', '!'
	};
	readOnlyMemoryIndices.push_back({0u, 13u});
	testInstructions = {
		Instruction(0x41002A), //r0 = #42
		Instruction(0x2B0000), //COUT r0

		Instruction(0xFB0000), //COUT \n

		Instruction(0xBB0000), //PRINT "CFAB is neat!" (ROM0)
	};
	suppressDebug = true;

	std::streambuf* cbuf = std::cout.rdbuf(); //Backup COUT.
	std::stringstream capturedCOUT;
	std::cout.rdbuf(capturedCOUT.rdbuf());

	CFAB::runInstructionSet(testInstructions);

	std::cout.rdbuf(cbuf); //Reset to usual.
	std::string result = capturedCOUT.str(); //Fetch COUT'd text
	//Expected raw: "42\nCFAB is neat!"

	//Test if COUT contained correct data.
	bool PRINTsuccess = true;
	for (unsigned int idx=0u; idx<13u; idx++) {
		if (result[3u+idx] != readOnlyMemory[idx]) {
			PRINTsuccess = false;
			break;
		}
	}

	assertOrThrow(
		(result[0u] == '4') && (result[1u] == '2'),
		"Failed to display register value in console."
	);
	assertOrThrow(
		result[2u] == '\n',
		"Failed to emit newline character."
	);
	assertOrThrow(
		PRINTsuccess,
		"Failed to display ROM segment in console."
	);
}

//////// Other ////////







const std::vector<std::function<void()>> tests = {
	//Memory
	testSETImmediate, testSETRegister, testMOV, testRegisterClear,
	testRAMwrite, testRAMread, testROMCopy, testRAMManagement,

	//Maths
	testADD, testSUB, testMUL, testDIV,
	testMOD, testINV, testABS,
	testSHF_L, testSHF_R, testSequenceArithmetic,

	//Logic
	testAND, testOR, testEQU, testXOR,
	testComparisons, testSequenceLogic,

	//Other
	testBRN, testExit, testInput, testOutput,
	testOverflows, testPRINTandCOUTs
};


void doTests() {
	//Tests specific cases using assertOrThrow.

	numPassed = 0u;
	//Main tests
	for (std::function<void()> test : tests) {
		std::fill(registers, registers + REG_COUNT, static_cast<int8_t>(0)); //Clear all registers.
		std::fill(randomAccessMemory, randomAccessMemory + RAM_COUNT, static_cast<int8_t>(0)); //Clear all RAM.
		readOnlyMemory.clear();
		readOnlyMemoryIndices.clear();
		programCounter = 0u;
		run = true;

		try {
			test();
			std::cout << "\033[32G\033[1;32m[TEST PASSED]\033[0;m" << std::endl;
		} catch (const std::exception& e) {
			std::cerr << "\033[32G\033[1;31m[TEST FAILED]\033[0;m : \033[1;33m" << e.what() << "\033[0;m" << std::endl;
		}
	}

	std::cout << std::endl << std::format(
		"\033[1;33m[RESULTS] ({:.0f}%) \033[0;m: ",
		100.0f * static_cast<float>(numPassed) / static_cast<float>(numRan)
	);
	std::cout << std::format("\033[1;32mPASSED: {}, ", numPassed);
	std::cout << std::format("\033[1;31mFAILED: {}\033[0;m", numRan - numPassed) << std::endl;
}

}


#endif

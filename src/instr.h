#ifndef INSTR_H
#define INSTR_H

#include "includes.h"
#include "constants.h"
#include "utils.h"
#include "graphics.h"





/*
FB_ --> 2 flag-bits in instr; values 0-3.
A/B --> The values in the instruction; can be register addresses or immediate values.
Ia/Ib --> Immediate operand A/B. 0: register index. 1: Immediate value.
INSTR --> Instruction opcode (see below table)

Bit layout;
Ia | Ib | F B | O P C O D E |
 b |  b | b b |   b b b b   |

 MNE | BITS | HEX | DESCRIPTION
-----+------+-----+-------------
 NOP | 0000 |  0  | Blank, No-Op instruction
 SET | 0001 |  1  | Sets a register to some value. If 2nd value is reg, copy data.
 MOV | 0010 |  2  | Moves contents of 1 register to another, resets source register to 0.
 ADD | 0011 |  3  | FB0, Adds 2 values. Can also be used as logical OR. FB1, bitwise OR.
 SUB | 0100 |  4  | Subtracts 2 values.
 MUL | 0101 |  5  | FB0, Multiplies 2 values. Can also be used as logical AND. FB1, bitwise AND.
 DIV | 0110 |  6  | Int-Divides 2 values. If denominator is 0, result is 0. (later -> div0 flag?)
 NOT | 0111 |  7  | FB0, logical NOT. FB1, bitwise NOT. FB2, invert number. FB3, abs(number)
 EQU | 1000 |  8  | FB0, A==B. FB1, A!=B. (!= is equivalent to XOR.) FB2, bitwise XOR. FB3, bitwise XNOR.
 GRT | 1001 |  9  | FB0, A>B. FB1, A<B. FB2, A>=B. FB3 A<=B. (bit 2 changes inclusivity, bit 1 changes func.)
 BRN | 1010 |  A  | FB0, Branch to (A<<8)|B if rOP!=0. FB1, unconditional branch to (A<<8)|B. FB2, same as FB0 but rOP==0.
 I_O | 1011 |  B  | FB0, Gets input 16, writing 8 to A and 8 to B [A/B MUST BE REGISTERS]. FB1, same but outputs immediate/register 16b. FB2, prints some value to console. FB3, prints ASCII char by index.
 SHF | 1100 |  C  | FB0, Left-shifts A by B bits. FB1, Right-shifts A by B bits.
 EXT | 1101 |  D  | Extra; FB0, halt. FB1, Clear all registers. FB2, write to RAM. FB3, read from RAM. [RAM uses rOP for read/write value.]
 SLP | 1110 |  E  | Sleep; FB0, sleep for (A<<8)|B milliseconds. FB1, sleeps until user input (should be paired with I_O call after)
 MEM | 1111 |  F  | FB0, Clears RAM section. FB1, Copy RAM section. FB2, Loads ROM segment into RAM.
*/




namespace CFAB {



#define B16(instr) ((static_cast<uint16_t>(instr.A) << 8) | static_cast<uint16_t>(instr.B))


void displayInstruction(const Instruction& instr) {
	if (/*!silenceDebug && */verbose) {
		const std::array<std::string, 16> opcodeMap = {
			"NOP", "SET", "MOV", "ADD",
			"SUB", "MUL", "DIV", "NOT",
			"EQU", "GRT", "BRN", "I_O",
			"SHF", "EXT", "SLP", "MEM"
		};
		const std::array<std::string, 4> extMap = {
			"HALT", "CLEAR", "RAM-WRITE", "RAM-READ"
		};
		const std::array<std::string, 4> i_oMap = {
			"INPUT", "OUTPUT", "COUT", "I_O"
		};

		std::string opcodeName = opcodeMap[instr.opcode];
		if (opcodeName == "EXT") {
			opcodeName = extMap[instr.flags];
		} else if (opcodeName == "I_O") {
			opcodeName = i_oMap[instr.flags];
		}

		std::cout << "\033[0;mBytes: \033[1;35m0x" << std::hex << instr.raw << "\033[0;m, Instruction: \033[1;35m" << opcodeName;
		std::cout << "\033[0;m, Flag-Bits: \033[1;35m" << std::bitset<2>(instr.flags).to_string();
		if (instr.Aimmediate) {
			std::cout << "\033[0;m, A-value: \033[1;36m" << std::to_string(instr.A);
		} else {
			uint8_t registerIndexA = static_cast<uint8_t>(maths::clamp(static_cast<unsigned int>(instr.Aptr - registers), 0u, REG_COUNT-1u));
			std::cout << "\033[0;m, A-index: \033[1;36mr" << std::to_string(registerIndexA) << " [" << std::to_string(registers[registerIndexA]) << "]";
		}
		if (instr.Bimmediate) {
			std::cout << "\033[0;m, B-value: \033[1;36m" << std::to_string(instr.B);
		} else {
			uint8_t registerIndexB = static_cast<uint8_t>(maths::clamp(static_cast<unsigned int>(instr.Bptr - registers), 0u, REG_COUNT-1u));
			std::cout << "\033[0;m, B-index: \033[1;36mr" << std::to_string(registerIndexB) << " [" << std::to_string(registers[registerIndexB]) << "]";
		}

		std::cout << "\033[0;m" << std::endl;
	}
}




static bool needsNewLN;
void runInstructionSet(std::vector<Instruction>& instructionData, Instruction** endPTR=nullptr) {
	std::chrono::time_point<std::chrono::high_resolution_clock> start;
	if (checkSpeed) {
		start = std::chrono::high_resolution_clock::now();
	}


	Instruction* instrPTR = instructionData.data();
	Instruction* end = instrPTR + instructionData.size();

	static const void* dispatchTable[16] = {
		&&NOP_LABEL, &&SET_LABEL, &&MOV_LABEL, &&ADD_LABEL,
		&&SUB_LABEL, &&MUL_LABEL, &&DIV_LABEL, &&NOT_LABEL,
		&&EQU_LABEL, &&GRT_LABEL, &&BRN_LABEL, &&I_O_LABEL,
		&&SHF_LABEL, &&EXT_LABEL, &&SLP_LABEL, &&MEM_LABEL
	};


#define DISPATCH() \
	do { \
		if ((instrPTR >= end) || (!run)) {goto VM_EXIT;} \
		instr = *(instrPTR++); \
		numExecuted++; \
		displayInstruction(instr); \
		if (!instr.Aimmediate) {instr.A = *instr.Aptr;} \
		if (!instr.Bimmediate) {instr.B = *instr.Bptr;} \
		goto **(dispatchTable + instr.opcode); \
	} while(0)

	Instruction instr;
	int8_t result = 0;
	run = true;

	goto VM_ENTRY;











//////// OPCODES ////////
NOP_LABEL: {DISPATCH();} //Do nothing





SET_LABEL: { //Set or copy register values
	if (!instr.Aimmediate) {
		*instr.Aptr = instr.B;
	}
	DISPATCH();
}





MOV_LABEL: { //Move register contents
	if ((!instr.Aimmediate) && (!instr.Bimmediate)) {
		*instr.Bptr = *instr.Aptr;
		*instr.Aptr = 0;
	}
	DISPATCH();
}





ADD_LABEL: { //Add 2 values	
	int8_t* result = &registers[REG_RESULT];
	if (instr.flags & 0b01) {
		(*result) = static_cast<int8_t>(static_cast<uint8_t>(instr.A) | static_cast<uint8_t>(instr.B));
	} else {
		(*result) = instr.A + instr.B;
	}

	DISPATCH();
}





SUB_LABEL: { //Subtract 2 values
	registers[REG_RESULT] = instr.A - instr.B;

	DISPATCH();
}





MUL_LABEL: { //Multiply 2 values	
	int8_t* result = &registers[REG_RESULT];
	if (instr.flags & 0b01) {
		(*result) = static_cast<int8_t>(static_cast<uint8_t>(instr.A) & static_cast<uint8_t>(instr.B));
	} else {
		(*result) = instr.A * instr.B;
	}

	DISPATCH();
}





DIV_LABEL: { //Divide 2 values
	if (instr.B != 0) { //Can't divide by zero.
		int8_t* result = &registers[REG_RESULT];
		if (instr.flags & 0b01) {
			(*result) = instr.A % instr.B;
		} else {
			(*result) = instr.A / instr.B;
		}
	}

	DISPATCH();
}





NOT_LABEL: { //Inverts or gets opposite.
	int8_t* result = &registers[REG_RESULT];
	switch (instr.flags) {
		case 0b00: { //Logical NOT.
			(*result) = (instr.A & BITS_1) ? 0 : 1;
			break;
		}
		case 0b01: { //Bitwise NOT.
			(*result) = ~instr.A;
			break;
		}
		case 0b10: { //Invert number.
			(*result) = -instr.A;
			break;
		}
		case 0b11: { //Absolute value of number.
			(*result) = std::abs(instr.A);
			break;
		}
	}
	DISPATCH();
}





EQU_LABEL: { //A==B, A!=B, A^B.
	int8_t* result = &registers[REG_RESULT];
	switch (instr.flags) {
		case 0b00: { //A == B
			(*result) = instr.A == instr.B;
			break;
		}
		case 0b01: { //A != B
			(*result) = instr.A != instr.B;
			break;
		}
		case 0b10: { //Bitwise XOR
			(*result) = instr.A ^ instr.B;
			break;
		}
		case 0b11: { //Bitwise XNOR
			(*result) = ~(instr.A ^ instr.B);
			break;
		}
	}

	DISPATCH();
}





GRT_LABEL: { //A>B, A<B, inclusive/exclusive.
	int8_t intermediate;
	if (instr.flags & 0b01) { //Less-than
		intermediate = instr.A < instr.B;
	} else { //Greater-than
		intermediate = instr.A > instr.B;
	}

	//Inclusive/exclusive GRT/LSS, controlled by bit "2" being enabled.
	registers[REG_RESULT] = intermediate || ((instr.flags & 0b10) && (instr.A == instr.B));

	DISPATCH();
}





BRN_LABEL: { //Branch conditional/unconditional.
	bool BRNif0 = instr.flags & 0b01;
	bool conditional = instr.flags & 0b10;

	if (
		(!conditional) || //JMP, unconditional
		((registers[REG_RESULT] != 0) && (!BRNif0)) || //BRN-If-1
		(!(registers[REG_RESULT] != 0) && (BRNif0))    //BRN-If-0.
	) {
		instrPTR = instructionData.data() + B16(instr);
	}

	DISPATCH();
}






I_O_LABEL: { //Get input/Set output
	switch (instr.flags) {
		case 0b00: { //Input
			if (instr.Aimmediate || instr.Bimmediate) {break; /* Do not allow. */}
			//Get inputs and write to registers A and B.
			(*instr.Aptr) = static_cast<int8_t>((inputBits >> 8u) & BITS_8);
			(*instr.Bptr) = static_cast<int8_t>((inputBits >> 0u) & BITS_8);
			break;
		}

		case 0b01: { //Output
			//Write values of A and B to the output bits.
			//Not a good plan to cast twice, but needs to change negative values to their 8-bit complement representation, then increase to 16.
			outputBits = (static_cast<uint8_t>(instr.A) << 8) | static_cast<uint8_t>(instr.B);
			break;
		}

		case 0b10: { //std::cout call, effectively.
			std::cout << std::to_string(instr.A) << std::flush;
			break;
		}

		case 0b11: { //Prints char from given char-set.
			if (!instr.Aimmediate) { //Single "dynamic" char from memory [older method]
				uint8_t index = static_cast<uint8_t>(instr.A);
				if (index == 0x0Au) {
					//Newline char
					std::cout << std::endl;
					needsNewLN = false;
				} else if (index < 0xFFu) {
					std::cout << char(index) << std::flush;
					needsNewLN = true;
				}

			} else if (instr.Bimmediate) {
				//Newline cmd
				std::cout << std::endl;
				needsNewLN = false;

			} else { //Read longer text from ROM.
				uint8_t ROMsegmentIndex = static_cast<uint8_t>(instr.A);

				std::pair<uint16_t, uint16_t> ROMindexPair = readOnlyMemoryIndices[ROMsegmentIndex];
				uint16_t numberOfCharacters = ROMindexPair.second - ROMindexPair.first;
				
				std::cout.write(
					reinterpret_cast<const char*>(&readOnlyMemory[ROMindexPair.first]),
					numberOfCharacters
				);
			}
		}
	}
	DISPATCH();
}





SHF_LABEL: { //Bitshift A by B.
	int8_t* result = &registers[REG_RESULT];

	bool RSH = instr.flags & 0b01;
	if (instr.B < 0) {RSH = !RSH; /* Negative RSH means LSH, and vice-versa. */};

	if (RSH) {
		(*result) = static_cast<int8_t>(static_cast<uint8_t>(instr.A) >> instr.B);
	} else {
		(*result) = static_cast<int8_t>(static_cast<uint8_t>(instr.A) << instr.B);
	}

	DISPATCH();	
}





EXT_LABEL: { //Extra lesser-used commands.
	switch (instr.flags) {
		case 0b00: { //HALT
			run = false;
		}

		case 0b01: { //Clear all registers to value in operand A.
			std::fill(registers, registers + REG_COUNT, instr.A);
		}
		
		case 0b10: { //RAMwrite
			//Writes value in result register to RAM address (A<<8)|B
			uint16_t RAMaddr = B16(instr) & BITS_RAM;
			randomAccessMemory[RAMaddr] = static_cast<uint8_t>(registers[REG_RESULT]);
		}

		case 0b11: { //RAMread
			//Reads value from RAM address (A<<8)|B to result register
			uint16_t RAMaddr = B16(instr) & BITS_RAM;
			registers[REG_RESULT] = static_cast<int8_t>(randomAccessMemory[RAMaddr]);
		}
	}

	DISPATCH();
}





SLP_LABEL: { //Sleep until event, or for specified time.
	switch (instr.flags) {
		case 0b00: { //Wait for user input
			//TBA
		}

		case 0b01: { //Wait specified number of ms
			unsigned int sleepMS = B16(instr);
			std::this_thread::sleep_for(std::chrono::milliseconds(sleepMS));
		}

		case 0b10: { //Update the screen.
			graphics::drawCurrentScreen();
		}
	}

	DISPATCH();
}





MEM_LABEL: { //Bulk memory management.
	uint16_t RAMaddr = B16(instr) & BITS_RAM;
	int8_t* result = &registers[REG_RESULT];

	switch (instr.flags) {
		case 0b00: { //Clear section of RAM.
			int8_t clearValue = registers[REG_X];
			uint16_t RAMend = ((registers[REG_Y] << 8) | registers[REG_Z]) & BITS_RAM;
			//Start at RAMaddr, end at RAMend.
			std::fill(
				std::next(randomAccessMemory, RAMaddr),
				std::next(randomAccessMemory, RAMend),
				clearValue
			);
			(*result) = 1; //Success
			break;
		}

		case 0b01: { //Copy section of RAM.
			uint16_t RAMend = ((registers[REG_X] << 8) | registers[REG_Y]) & BITS_RAM;
			uint16_t RAMnew = ((registers[REG_Z] << 8) | registers[REG_W]) & BITS_RAM;

			uint16_t copySize = RAMend - RAMaddr;
			uint16_t freeSpace = RAM_COUNT - RAMnew;
			if (copySize > freeSpace) {
				break; //Fail
			}

			std::copy_n(
				std::next(randomAccessMemory, RAMaddr),
				copySize,
				std::next(randomAccessMemory, RAMnew)
			);

			(*result) = 1; //Success
			break;
		}

		case 0b10: { //Copy section of ROM into RAM.
			uint8_t ROMsegmentIndex = registers[REG_X];

			if (ROMsegmentIndex >= readOnlyMemoryIndices.size()) {
				break; //Fail
			}
			std::pair<uint16_t, uint16_t> ROMindexPair = readOnlyMemoryIndices[ROMsegmentIndex];
			uint16_t copySize = ROMindexPair.second - ROMindexPair.first;
			uint16_t freeSpace = RAM_COUNT - RAMaddr;
			if (copySize > freeSpace) {
				break; //Fail
			}

			std::copy_n(
				std::next(readOnlyMemory.begin(), ROMindexPair.first),
				copySize,
				std::next(randomAccessMemory, RAMaddr)
			);
			
			(*result) = 1; //Success
			break;
		}
	}
	
	DISPATCH();
}



VM_ENTRY: {DISPATCH();} //Enter.



VM_EXIT: { //Exit.
	if (needsNewLN) {std::cout << std::endl;}

	if (endPTR) {*endPTR = instrPTR;}

	if (checkSpeed) {
		std::chrono::time_point<std::chrono::high_resolution_clock> end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> elapsed = end - start;
		double freq = static_cast<double>(numExecuted)*1000.0f/elapsed.count();
		std::string freqStr = (freq > 1.0e3f) ? std::to_string(freq / 1000.0f)+"k" : std::to_string(freq);

		std::cout << std::endl;
		std::cout << "\033[1;30mExecuted:  \033[1;35m" << std::to_string(numExecuted) << " instructions\033[0;m" << std::endl;
		std::cout << "\033[1;30mElapsed:   \033[1;35m" << elapsed.count() << "ms\033[0;m" << std::endl;
		std::cout << "\033[1;30mFrequency: \033[1;35m" << freqStr << "Hz\033[0;m" << std::endl;
		std::cout << std::endl;
	}


	return;
}
//////// OPCODES ////////
}}



#endif

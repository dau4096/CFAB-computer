#ifndef INSTR_H
#define INSTR_H

#include "includes.h"
#include "constants.h"
#include "utils.h"




namespace CFAB {

inline unsigned int accessBit(const unsigned int& bits, const unsigned int index) {
	return (bits >> index) & BITS_1;
}

std::array<int8_t, 2> operandsTMP;
inline int8_t* getOperand(const unsigned int& instruction, const unsigned int index, bool* isImmediate) {
	int8_t operand = static_cast<int8_t>((instruction >> (1u-index)*8u) & BITS_8);
	*isImmediate = accessBit(instruction, 23u-index);
	if (*isImmediate) { //Not a register index, has specific value.
		int oIdx = static_cast<int>(index);
		operandsTMP[oIdx] = operand;
		return &(operandsTMP[oIdx]);
	} else { //Register index 0-127.
		uint8_t registerIndex = static_cast<uint8_t>(maths::clamp(static_cast<unsigned int>(operand), 0u, REG_COUNT-1u));
		return &(registers[registerIndex]);
	}
}


inline uint16_t get16Bit(int8_t* Aptr, int8_t* Bptr) {
	return (static_cast<uint16_t>(static_cast<uint8_t>(*Aptr)) << 8u) | static_cast<uint16_t>(static_cast<uint8_t>(*Bptr));
}


bool needsNewLN;
bool executeInstruction(const unsigned int instruction, int8_t* result, bool silenceDebug=false) {
/*
FB_ --> 2 flag-bits in instr; values 0-3.
A/B --> The values in the instruction; can be register addresses or immediate values.
Ia/Ib --> Immediate operand A/B. 0: register index. 1: Immediate value.
INSTR --> Instruction opcode (see below table)

Bit layout;
Ia | Ib | F B | I N S T R |
 1 |  1 | 1 1 |  1 1 1 1  |

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
 __F | 1111 |  F  | 
*/

	bool Aimmediate, Bimmediate;
	int8_t* Aptr = getOperand(instruction, 0u, &Aimmediate); //First operand
	int8_t* Bptr = getOperand(instruction, 1u, &Bimmediate); //Second operand
	uint8_t opcode = (instruction >> 16u) & BITS_4;
	uint8_t flagBits = (instruction >> 20) & BITS_2;

	bool returnsValue = false;


	if (!silenceDebug && verbose) {
		const std::array<std::string, 16> opcodeMap = {
			"NOP", "SET", "MOV", "ADD",
			"SUB", "MUL", "DIV", "NOT",
			"EQU", "GRT", "BRN", "I_O",
			"SHF", "EXT", "SLP", "__F"
		};
		const std::array<std::string, 4> extMap = {
			"HALT", "CLEAR", "RAM-WRITE", "RAM-READ"
		};
		const std::array<std::string, 4> i_oMap = {
			"INPUT", "OUTPUT", "COUT", "I_O"
		};

		std::string opcodeName = opcodeMap[opcode];
		if (opcodeName == "EXT") {
			opcodeName = extMap[flagBits];
		} else if (opcodeName == "I_O") {
			opcodeName = i_oMap[flagBits];
		}

		std::cout << "\033[0;mBytes: \033[1;35m0x" << std::hex << instruction << "\033[0;m, Instruction: \033[1;35m" << opcodeName;
		std::cout << "\033[0;m, Flag-Bits: \033[1;35m" << std::to_string(flagBits);
		if (Aimmediate) {
			std::cout << "\033[0;m, A-value: \033[1;36m" << std::to_string(*Aptr);
		} else {
			int8_t operandA = static_cast<int8_t>((instruction >> 8u) & BITS_8);
			uint8_t registerIndexA = static_cast<uint8_t>(maths::clamp(static_cast<unsigned int>(operandA), 0u, REG_COUNT-1u));
			std::cout << "\033[0;m, A-index: \033[1;36mr" << std::to_string(registerIndexA) << " [" << std::to_string(registers[registerIndexA]) << "]";
		}
		if (Bimmediate) {
			std::cout << "\033[0;m, B-value: \033[1;36m" << std::to_string(*Bptr);
		} else {
			int8_t operandB = static_cast<int8_t>((instruction >> 0u) & BITS_8);
			uint8_t registerIndexB = static_cast<uint8_t>(maths::clamp(static_cast<unsigned int>(operandB), 0u, REG_COUNT-1u));
			std::cout << "\033[0;m, B-index: \033[1;36mr" << std::to_string(registerIndexB) << " [" << std::to_string(registers[registerIndexB]) << "]";
		}

		std::cout << "\033[0;m | ";
	}


	switch (opcode) {
		case NOP: { //Do nothing
			break;
		}

		case SET: { //Set or copy register values
			if (Aimmediate) {break; /* Do not allow. */}
			(*Aptr) = (*Bptr);
			break;
		}

		case MOV: { //Move register contents
			if (Aimmediate || Bimmediate) {break; /* Do not allow. */}
			(*Bptr) = (*Aptr);
			(*Aptr) = 0;
			break;
		}

		case ADD: { //Add 2 values
			if (accessBit(flagBits, 0u)) { //Bitwise OR
				(*result) = (*Aptr) | (*Bptr);
			} else { //Add / Logical OR.
				(*result) = (*Aptr) + (*Bptr);
			}
			returnsValue = true;
			break;
		}

		case SUB: { //Subtract 2 values
			(*result) = (*Aptr) - (*Bptr);
			returnsValue = true;
			break;
		}

		case MUL: { //Multiply 2 values
			if (accessBit(flagBits, 0u)) { //Bitwise AND
				(*result) = (*Aptr) & (*Bptr);
			} else { //Multiply / Logical AND.
				(*result) = (*Aptr) * (*Bptr);
			}
			returnsValue = true;
			break;
		}

		case DIV: { //Divide 2 values
			if (*Bptr) { //Nonzero divisor
				if (accessBit(flagBits, 0u)) { //Modulus
					(*result) = (*Aptr) % (*Bptr);
				} else { //Division
					(*result) = (*Aptr) / (*Bptr);
				}
			} else { //Div-0
				(*result) = 0;
			}
			returnsValue = true;
			break;
		}

		case NOT: { //Inverts or gets opposite.
			switch (flagBits) {
				case 0u: { //Logical NOT.
					(*result) = (accessBit(*Aptr, 0)) ? 0 : 1;
					break;
				}
				case 1u: { //Bitwise NOT.
					(*result) = ~(*Aptr);
					break;
				}
				case 2u: { //Invert number.
					(*result) = -(*Aptr);
					break;
				}
				case 3u: { //Absolute value of number.
					(*result) = std::abs(*Aptr);
					break;
				}
			}
			returnsValue = true;
			break;
		}

		case EQU: { //A==B, A!=B, A^B.
			switch (flagBits) {
				case 0u: { //A == B
					(*result) = (*Aptr) == (*Bptr);
					break;
				}
				case 1u: { //A != B
					(*result) = (*Aptr) != (*Bptr);
					break;
				}
				case 2u: { //Bitwise XOR
					(*result) = (*Aptr) ^ (*Bptr);
					break;
				}
				case 3u: { //Bitwise XNOR
					(*result) = ~((*Aptr) ^ (*Bptr));
					break;
				}
			}

			returnsValue = true;
			break;
		}

		case GRT: { //A>B, A<B, inclusive/exclusive.
			unsigned int intermediate;
			if (accessBit(flagBits, 0u)) { //Less-than
				intermediate = (*Aptr) < (*Bptr);
			} else { //Greater-than
				intermediate = (*Aptr) > (*Bptr);
			}
			//Inclusive/exclusive GRT/LSS.
			(*result) = intermediate || (accessBit(flagBits, 1u) && ((*Aptr) == (*Bptr)));
			returnsValue = true;
			break;
		}

		case BRN: { //Branch conditional/unconditional.
			bool BRNif0 = accessBit(flagBits, 0u);
			bool conditional = accessBit(flagBits, 1u);
			if (
				!conditional || //JMP, unconditional
				((registers[REG_RESULT]!=0) && !BRNif0) || //BRN-If-1
				(!(registers[REG_RESULT]!=0) && BRNif0)    //BRN-If-0.
			) {
				programCounter = get16Bit(Aptr, Bptr);
			}
			break;
		}

		case I_O: { //Get input/Set output
			switch (flagBits) {
				case 0u: { //Input
					if (Aimmediate || Bimmediate) {break; /* Do not allow. */}
					//Get inputs and write to registers A and B.
					(*Aptr) = static_cast<int8_t>((inputBits >> 8u) & BITS_8);
					(*Bptr) = static_cast<int8_t>((inputBits >> 0u) & BITS_8);
					break;
				}
				case 1u: { //Output
					//Write values of A and B to the output bits.
					//Not a good plan to cast twice, but needs to change negative values to their 8-bit complement representation, then increase to 16.
					outputBits = get16Bit(Aptr, Bptr);
					break;
				}
				case 2u: { //std::cout call, effectively.
					int8_t operandA = static_cast<int8_t>((instruction >> 8u) & BITS_8);
					uint8_t registerIndexA = static_cast<uint8_t>(maths::clamp(static_cast<unsigned int>(operandA), 0u, REG_COUNT-1u));
					std::cout << std::to_string(*Aptr) << std::flush;
					break;
				}
				case 3u: { //Prints char from given char-set.
					const std::string charSet = "0123456789 abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ?+-*/!^%&|=()[]~@'`<>,.:;";
					uint8_t index = static_cast<uint8_t>(*Aptr);
					if (index < charSet.length()) {
						std::cout << charSet.at(index) << std::flush;
						needsNewLN = true;
					} else if (index == charSet.length()) {
						//Newline char
						std::cout << std::endl;
						needsNewLN = false;
					}
				}
			}
			if (flagBits == 2u) { 

			} else if (accessBit(flagBits, 0u)) { 
			} else { 
			}
			break;
		}

		case SHF: { //Bitshift A by B.
			bool RSH = accessBit(flagBits, 0u);
			if ((*Bptr) < 0) {RSH = !RSH;};
			if (RSH){
				(*result) = (*Aptr) >> (*Bptr);
			} else {
				(*result) = (*Aptr) << (*Bptr);
			}
			returnsValue = true;
			break;			
		}

		case EXT: { //Extra lesser-used commands.
			switch (flagBits) {
				case 0u: { //HALT
					run = false;
					break;
				}
				case 1u: { //Clear all registers to value in operand A.
					std::fill(registers.begin(), registers.end(), *Aptr);
					break;
				}
				case 2u: { //RAMwrite
					//Writes value in result register to RAM address (A<<8)|B
					uint16_t RAMaddr = get16Bit(Aptr, Bptr) & BITS_12;
					randomAccessMemory[RAMaddr] = registers[REG_RESULT];
					break;
				}
				case 3u: { //RAMread
					//Reads value from RAM address (A<<8)|B to result register
					uint16_t RAMaddr = get16Bit(Aptr, Bptr) & BITS_12;
					(*result) = randomAccessMemory[RAMaddr];
					returnsValue = true;
					break;
				}
			}
			break;
		}

		case SLP: { //Sleep until event, or for specified time.
			if (accessBit(flagBits, 0u)) { //Wait for user input
			} else { //Wait specified number of ms
				unsigned int sleepMS = get16Bit(Aptr, Bptr);
				std::this_thread::sleep_for(std::chrono::milliseconds(sleepMS));
			}
			break;
		}


		case __F: { //Currently unassigned, acts as NOP.
			break;
		}

	}


	if (!silenceDebug && verbose) {
		if (returnsValue) {
			std::cout << "\033[1;33mReturned: " << std::to_string(*result) << "\033[0;m";
		}
		std::cout << std::endl;
	}


	return returnsValue;

}


void runInstructionSet(std::vector<unsigned int>& instructionData) {
	std::chrono::time_point<std::chrono::high_resolution_clock> start;
	if (checkSpeed) {
		start = std::chrono::high_resolution_clock::now();
	}

	int8_t result;
	needsNewLN = false;
	while (programCounter < instructionData.size()) {
		unsigned int instruction = instructionData[programCounter];
		programCounter++;
		numExecuted++;

		//run instr;
		bool gaveResult = executeInstruction(
			instruction, &result, false
		);
		if (gaveResult) {
			registers[REG_RESULT] = result;
		}
		if (!run) {break;}
	}
	run = false;
	if (needsNewLN) {std::cout << std::endl;}


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
}

}


#endif

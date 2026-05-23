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
 MEM | 1111 |  F  | 
*/


namespace CFAB {



#define B16(instr) ((static_cast<uint16_t>(instr.A) << 8) | static_cast<uint16_t>(instr.B))


bool needsNewLN;
bool executeInstruction(Instruction& instr, int8_t* result, bool silenceDebug=false) {
	bool returnsValue = false;

	if (!instr.Aimmediate) {instr.A = *instr.Aptr;}
	if (!instr.Bimmediate) {instr.B = *instr.Bptr;}

	if (!silenceDebug && verbose) {
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
		std::cout << "\033[0;m, Flag-Bits: \033[1;35m" << std::to_string(instr.flags);
		if (instr.Aimmediate) {
			std::cout << "\033[0;m, A-value: \033[1;36m" << std::to_string(instr.A);
		} else {
			int8_t operandA = static_cast<int8_t>((instr.raw >> 8u) & BITS_8);
			uint8_t registerIndexA = static_cast<uint8_t>(maths::clamp(static_cast<unsigned int>(operandA), 0u, REG_COUNT-1u));
			std::cout << "\033[0;m, A-index: \033[1;36mr" << std::to_string(registerIndexA) << " [" << std::to_string(registers[registerIndexA]) << "]";
		}
		if (instr.Bimmediate) {
			std::cout << "\033[0;m, B-value: \033[1;36m" << std::to_string(instr.B);
		} else {
			int8_t operandB = static_cast<int8_t>((instr.raw >> 0u) & BITS_8);
			uint8_t registerIndexB = static_cast<uint8_t>(maths::clamp(static_cast<unsigned int>(operandB), 0u, REG_COUNT-1u));
			std::cout << "\033[0;m, B-index: \033[1;36mr" << std::to_string(registerIndexB) << " [" << std::to_string(registers[registerIndexB]) << "]";
		}

		std::cout << "\033[0;m | ";
	}


	switch (instr.opcode) {
		case NOP: { //Do nothing
			break;
		}

		case SET: { //Set or copy register values
			if (instr.Aimmediate) {break; /* Do not allow. */}
			*instr.Aptr = instr.B;
			break;
		}

		case MOV: { //Move register contents
			if (instr.Aimmediate || instr.Bimmediate) {break; /* Do not allow. */}
			*instr.Bptr = instr.A;
			*instr.Aptr = 0;
			break;
		}

		case ADD: { //Add 2 values
			if (instr.flags & 0b01) { //Bitwise OR
				(*result) = instr.A | instr.B;
			} else { //Add / Logical OR.
				(*result) = instr.A + instr.B;
			}
			returnsValue = true;
			break;
		}

		case SUB: { //Subtract 2 values
			(*result) = instr.A - instr.B;
			returnsValue = true;
			break;
		}

		case MUL: { //Multiply 2 values
			if (instr.flags & 0b01) { //Bitwise AND
				(*result) = instr.A & instr.B;
			} else { //Multiply / Logical AND.
				(*result) = instr.A * instr.B;
			}
			returnsValue = true;
			break;
		}

		case DIV: { //Divide 2 values
			if (instr.B) { //Nonzero divisor
				if (instr.flags & 0b01) { //Modulus
					(*result) = instr.A % instr.B;
				} else { //Division
					(*result) = instr.A / instr.B;
				}
			} else { //Div-0
				(*result) = 0;
			}
			returnsValue = true;
			break;
		}

		case NOT: { //Inverts or gets opposite.
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
			returnsValue = true;
			break;
		}

		case EQU: { //A==B, A!=B, A^B.
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

			returnsValue = true;
			break;
		}

		case GRT: { //A>B, A<B, inclusive/exclusive.
			unsigned int intermediate;
			if (instr.flags & 0b01) { //Less-than
				intermediate = instr.A < instr.B;
			} else { //Greater-than
				intermediate = instr.A > instr.B;
			}
			//Inclusive/exclusive GRT/LSS.
			(*result) = intermediate || ((instr.flags & 0b10) && (instr.A == instr.B));
			returnsValue = true;
			break;
		}

		case BRN: { //Branch conditional/unconditional.
			bool BRNif0 = instr.flags & 0b01;
			bool conditional = instr.flags & 0b10;
			if (
				!conditional || //JMP, unconditional
				((registers[REG_RESULT]!=0) && !BRNif0) || //BRN-If-1
				(!(registers[REG_RESULT]!=0) && BRNif0)    //BRN-If-0.
			) {
				programCounter = B16(instr);
			}
			break;
		}

		case I_O: { //Get input/Set output
			switch (instr.flags) {
				case 0b00: { //Input
					if (instr.Aimmediate || instr.Bimmediate) {break; /* Do not allow. */}
					//Get inputs and write to registers A and B.
					*instr.Aptr = static_cast<int8_t>((inputBits >> 8u) & BITS_8);
					*instr.Bptr = static_cast<int8_t>((inputBits >> 0u) & BITS_8);
					break;
				}
				case 0b01: { //Output
					//Write values of A and B to the output bits.
					outputBits = B16(instr);
					break;
				}
				case 0b10: { //std::cout call, effectively.
					uint8_t registerIndexA = maths::clamp(static_cast<unsigned int>(instr.A), 0u, REG_COUNT-1u);
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
						uint8_t ROMsegmentIndex = static_cast<uint16_t>(instr.A);

						std::pair<uint16_t, uint16_t> ROMindexPair = readOnlyMemoryIndices[ROMsegmentIndex];
						uint16_t numberOfCharacters = ROMindexPair.second - ROMindexPair.first;
						
						std::cout.write(
							reinterpret_cast<const char*>(&readOnlyMemory[ROMindexPair.first]),
							numberOfCharacters
						);
					}
				}
			}
			break;
		}

		case SHF: { //Bitshift A by B.
			bool RSH = instr.flags & 0b01;
			if (instr.B < 0) {RSH = !RSH;};
			if (RSH){
				(*result) = instr.A >> instr.B;
			} else {
				(*result) = instr.A << instr.B;
			}
			returnsValue = true;
			break;			
		}

		case EXT: { //Extra lesser-used commands.
			switch (instr.flags) {
				case 0b00: { //HALT
					run = false;
					break;
				}
				case 0b01: { //Clear all registers to value in operand A.
					std::fill(registers, registers + REG_COUNT, instr.A);
					break;
				}
				case 0b10: { //RAMwrite
					//Writes value in result register to RAM address (A<<8)|B
					uint16_t RAMaddr = B16(instr) & BITS_RAM;
					randomAccessMemory[RAMaddr] = registers[REG_RESULT];
					break;
				}
				case 0b11: { //RAMread
					//Reads value from RAM address (A<<8)|B to result register
					uint16_t RAMaddr = B16(instr) & BITS_RAM;
					(*result) = randomAccessMemory[RAMaddr];
					returnsValue = true;
					break;
				}
			}
			break;
		}

		case SLP: { //Sleep until event, or for specified time.
			switch (instr.flags) {
				case 0b00: { //Wait for user input
					//TBA
					break;
				}
				case 0b01: { //Wait specified number of ms
					unsigned int sleepMS = B16(instr);
					std::this_thread::sleep_for(std::chrono::milliseconds(sleepMS));
					break;
				}
				case 0b10: { //Update the screen.
					graphics::drawCurrentScreen();
					break;
				}
			}
			break;
		}


		case MEM: { //Bulk memory management.
			uint16_t RAMaddr = B16(instr) & BITS_RAM;
			*result = 0; //Default to no-success.

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
					*result = 1; //Success
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

					*result = 1; //Success
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
					
					*result = 1; //Success
					break;
				}
			}
			
			returnsValue = true;
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


void runInstructionSet(std::vector<Instruction>& instructionData) {
	std::chrono::time_point<std::chrono::high_resolution_clock> start;
	if (checkSpeed) {
		start = std::chrono::high_resolution_clock::now();
	}

	int8_t result;
	needsNewLN = false;
	while (programCounter < instructionData.size()) {
		Instruction& instruction = instructionData[programCounter];
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

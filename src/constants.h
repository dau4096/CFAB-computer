#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "includes.h"

using namespace std;


//////// GENERAL ////////
#define BITS_1 0x1
#define BITS_2 0x3
#define BITS_4 0xF
#define BITS_8 0xFF
#define BITS_16 0xFFFF
//////// GENERAL ////////

//////// DEBUG ////////
#define DEBUG
#define DEBUG_SHOW_OPERATIONS
#define DEBUG_RUN_TESTS
//////// DEBUG ////////


//////// OPCODES ////////
#define NOP 0x0u
#define SET 0x1u
#define MOV 0x2u
#define ADD 0x3u
#define SUB 0x4u
#define MUL 0x5u
#define DIV 0x6u
#define NOT 0x7u
#define EQU 0x8u
#define GRT 0x9u
#define BRN 0xAu
#define I_O 0xBu
#define SHF 0xCu
#define EXT 0xDu

//Not assigned yet;
#define __E 0xEu
#define __F 0xFu
//////// OPCODES ////////


//////// REGISTERS ////////
#define REG_COUNT 128u
#define REG_RESULT 127u /* Last index, stores result of last instruction if it returned one. */
//////// REGISTER ////////


#endif

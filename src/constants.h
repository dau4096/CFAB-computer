#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "includes.h"

using namespace std;





//////// GENERAL ////////

#define BITS_1 0x1
#define BITS_2 0x3
#define BITS_4 0xF
#define BITS_8 0xFF
#define BITS_12 0xFFF
#define BITS_16 0xFFFF

#define FILE_PATH "data/help.dat"

//////// GENERAL ////////





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
#define SLP 0xEu

//Not assigned yet;
#define __F 0xFu

//////// OPCODES ////////





//////// MEMORY ////////

//REGISTERS
#define REG_COUNT 64u /* 2^6 */
#define REG_RESULT 63u /* Last index, stores result of last instruction if it returned one. */


//RAM
#define RAM_COUNT 4096u /* 2^12 */
//SCREEN [RAM]
#define SCREEN_INDEX 3584u /* 0xE00, 512 bytes at the end of the RAM. */
#define SCREEN_WIDTH 32u
#define SCREEN_HEIGHT 16u

//////// MEMORY ////////


#endif

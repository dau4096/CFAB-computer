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

#define FALSE 0
#define TRUE 1

#define FILE_PATH "data/help.dat"
#define HEADER_LENGTH_BYTES 9u /* 72 bits. */

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
#define MEM 0xFu

//////// OPCODES ////////





//////// MEMORY ////////

//REGISTERS
#define REG_COUNT 64u /* 2^6 */
#define REG_RESULT 63u /* Last index, stores result of last instruction if it returned one. */
#define BITS_REG 0x3F
//Used when 16 bits of operand arent enough.
#define REG_Z 62u
#define REG_Y 61u
#define REG_X 60u
#define REG_W 59u


//RAM
#define RAM_COUNT 4096u /* 2^12 */
#define BITS_RAM BITS_12
//SCREEN [IN RAM]
#define SCREEN_WIDTH 32u
#define SCREEN_HEIGHT 16u
#define SCREEN_ELEMENT_SIZE (SCREEN_WIDTH*SCREEN_HEIGHT)
#define SCREEN_START_INDEX (RAM_COUNT-SCREEN_ELEMENT_SIZE) /* 0xE00, 512 values at the end of the RAM. */
enum GraphicsMode {
	GM_NONE,
	GM_TEXT,
	GM_256c,
	GM_RGBc
};
//When defined, will make the screen use double spaces rather than ½ square characters.
#define SCREEN_DOUBLE_SCALE
//////// MEMORY ////////


#endif

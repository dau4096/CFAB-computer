#pragma once

#include "includes.h"
#include "constants.h"

using namespace std;



inline uint16_t programCounter;
inline size_t numExecuted;
inline uint16_t inputBits, outputBits; //Used for user I/O.
inline int8_t registers[REG_COUNT]; //For short-term values.
inline int8_t randomAccessMemory[RAM_COUNT]; //Acts like a disk of sorts. RAM in name solely.
inline std::vector<std::pair<uint16_t, uint16_t>> readOnlyMemoryIndices; //Start/End indices for each ROM segment.
inline std::vector<int8_t> readOnlyMemory; //Taken from the end of the file.
inline glm::ivec2 consoleResolution;
inline GraphicsMode graphicsMode = GM_NONE;


struct MetaData {
	std::string filePath;
	uint8_t version;

	uint16_t numberOfInstructions;
	uint8_t graphicsModeIndex; //Acctually 2 bits.

	uint16_t numberOfROMSegments; //Acctually 14 bits.
};
inline MetaData metaData; //Metadata about this loaded file.


inline bool run;
inline bool verbose, checkSpeed = false; //CLI Arg-Parameters



struct Instruction {
	unsigned int raw;

	uint8_t opcode;
	uint8_t flags;

	bool Aimmediate;
	int8_t A;
	int8_t* Aptr;

	bool Bimmediate;
	int8_t B;
	int8_t* Bptr;

	Instruction()
		: raw(0x000000u), opcode(NOP), flags(0x00u),
		  Aimmediate(true), A(0), Aptr(nullptr),
		  Bimmediate(true), B(0), Bptr(nullptr) {}

	Instruction(unsigned int instr)
		 : raw(instr), opcode((instr >> 16u) & BITS_4), flags((instr >> 20u) & BITS_2),
		   Aimmediate(static_cast<bool>((instr >> 23u) & BITS_1)),
		   Bimmediate(static_cast<bool>((instr >> 22u) & BITS_1)) {
		   	uint8_t OPCa = (instr >> 8u) & BITS_8;
		   	Aptr = (Aimmediate) ? nullptr : registers + (OPCa & (REG_COUNT - 1u));
		   	A = (Aimmediate) ? static_cast<int8_t>(OPCa) : *Aptr;

		   	uint8_t OPCb = instr & BITS_8;
		   	Bptr = (Bimmediate) ? nullptr : registers + (OPCb & (REG_COUNT - 1u));
		   	B = (Bimmediate) ? static_cast<int8_t>(OPCb) : *Bptr;
		}
};



namespace maths {

template<typename T>
T clamp(T value, const T lower, const T upper) {
	return std::max(lower, std::min(value, upper));
}

}



namespace utils {

	static inline std::string strToUpper(const std::string& input) {
		std::string result = input;
		std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c){return std::toupper(c);});
		return result;
	}

	
	#define DEFAULT_CONSOLE_SIZE glm::ivec2(80, 24)
	inline glm::ivec2 getConsoleSizeChars() {
	#ifdef _WIN32
		return DEFAULT_CONSOLE_SIZE; //TODO: Replace later with windows.h method.
	#elif defined(__linux__)
		struct winsize ws;
		if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) { //Get window size in chars.
			return glm::ivec2(ws.ws_col, ws.ws_row);
		} else {return DEFAULT_CONSOLE_SIZE; /* Could not get current console size. */}
	#endif
	}

	inline glm::ivec2 getConsoleResolution() {
		return getConsoleSizeChars() * glm::ivec2(1, 2) - glm::ivec2(0, 2);
	}

}
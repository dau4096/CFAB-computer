#pragma once

#include "includes.h"
#include "constants.h"

using namespace std;



inline uint16_t programCounter;
inline size_t numExecuted;
inline uint16_t inputBits, outputBits; //Used for user I/O.
inline std::array<int8_t, REG_COUNT> registers; //For short-term values.
inline std::array<int8_t, RAM_COUNT> randomAccessMemory; //Acts like a disk of sorts. RAM in name solely.

inline bool run;
inline bool verbose, checkSpeed = false; //CLI Arg-Parameters




namespace maths {

template<typename T>
T clamp(T value, const T lower, const T upper) {
	return std::max(lower, std::min(value, upper));
}

}


namespace loader {


bool loadBytesData(const std::string& filePath, std::vector<uint8_t>& byteData) {
	std::ifstream dataFile(filePath, ios::binary | ios::ate);
	if (!dataFile) {
		std::cerr << "Failed to open file: " << filePath << std::endl;
		return false;
	}
	std::streamsize fileSize = dataFile.tellg();
	dataFile.seekg(0, ios::beg);
	byteData.resize(fileSize);

	if (!dataFile.read(reinterpret_cast<char*>(byteData.data()), fileSize)) {
		std::cerr << "Failed to read file: " << filePath << std::endl;
		return false;
	}
	return true;
}


bool loadInstructions(const std::string& filePath, std::vector<unsigned int>* instructionData) {
	//Read file
	std::vector<uint8_t> byteData;
	if (!loadBytesData(filePath, byteData)) {return false; /* Failed to read bytes. */}

	//24b instructions.
	//2.5 bytes
	uint16_t totalInstructions = static_cast<unsigned int>(std::floor(byteData.size() / 3)) & BITS_16; //3 Bytes each
	programCounter = 0u;
	run = true;
	for (unsigned int instructionIndex=0u; instructionIndex<totalInstructions; instructionIndex++) {
		unsigned int byteIndex = instructionIndex * 3;

		unsigned int instruction = {
			(static_cast<unsigned int>(byteData[byteIndex]) << 16u) |
			(static_cast<unsigned int>(byteData[byteIndex + 1u]) << 8u) |
			(static_cast<unsigned int>(byteData[byteIndex + 2u]))
		};

		instructionData->push_back(instruction);
	}

	return true;

}

}

#pragma once

#include "includes.h"
#include "constants.h"

using namespace std;



inline uint16_t totalInstructions;
inline uint16_t programCounter;
inline uint16_t inputBits, outputBits; //Used for user I/O.
inline std::array<int8_t, REG_COUNT> registers;
inline bool run;



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
	totalInstructions = static_cast<unsigned int>(std::floor(byteData.size() * 5 / 2)) & BITS_16;
	programCounter = 0u;
	run = true;
	for (unsigned int instructionIndex=0u; instructionIndex<totalInstructions; instructionIndex++) {
		unsigned int instruction;
		unsigned int byteIndex = std::floor(instructionIndex * 5 / 2);

		/*
		if ((instructionIndex % 2) > 0u) {
			//Odd index
			instruction = static_cast<unsigned int>(
				() |
				() |
				()
			);
		} else {
			//Even index

		}
		*/
		instructionData->push_back(instruction);
	}

	return true;

}

}

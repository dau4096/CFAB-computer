#pragma once

#include "includes.h"
#include "constants.h"

using namespace std;



inline uint16_t programCounter;
inline size_t numExecuted;
inline uint16_t inputBits, outputBits; //Used for user I/O.
inline std::array<int8_t, REG_COUNT> registers; //For short-term values.
inline std::array<int8_t, RAM_COUNT> randomAccessMemory; //Acts like a disk of sorts. RAM in name solely.


struct MetaData {
	std::string filePath;
	uint8_t version;

	uint16_t numberOfInstructions;
	uint8_t graphicsModeIndex;
};
inline MetaData metaData; //Metadata about this loaded file.


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



bool parseHeader(std::array<uint8_t, HEADER_LENGTH_BYTES>& header, const std::string& filePath, size_t numberOfReadBytes) {
	metaData.filePath = filePath;

	//Hex codes for encoded "CFAB" string.
	bool hasIdentifier = (
		(header[0] == 0x27u) && //"C"
		(header[1] == 0x2Au) && //"F"
		(header[2] == 0x25u) && //"A"
		(header[3] == 0x26u)    //"B"
	);

	metaData.version = header[4];
	bool versionValid = (metaData.version >= 2u); //Versions before 2 do not have a header.

	metaData.numberOfInstructions = static_cast<uint16_t>(header[5] << 8u) | static_cast<uint16_t>(header[6]);
	bool numInstructionsValid = (
		(metaData.numberOfInstructions > 0u) &&
		(numberOfReadBytes >= (HEADER_LENGTH_BYTES+(metaData.numberOfInstructions*3)))
	);

	metaData.graphicsModeIndex = header[7] & 0x3u; //First 2 bits of this byte.

	return hasIdentifier && versionValid && numInstructionsValid;

}


bool loadInstructions(const std::string& filePath, std::vector<unsigned int>* instructionData) {
	//Read file
	std::vector<uint8_t> byteData;
	if (!loadBytesData(filePath, byteData)) {return false; /* Failed to read bytes. */}


	//Parse the header.
	bool validFile;
	if (byteData.size() >= HEADER_LENGTH_BYTES) {
		std::array<uint8_t, HEADER_LENGTH_BYTES> header;
		std::copy_n(byteData.begin(), HEADER_LENGTH_BYTES, header.begin());
		validFile = parseHeader(header, filePath, byteData.size());

	} else {validFile = false;}
	if (!validFile) {std::cout << "File header invalid. If this is an old .dat file, please re-fabricate with a newer fabricator version." << std::endl; return false;}

	//Print metadata;
	std::cout << "FilePath: [" << metaData.filePath << "], Version " << std::to_string(metaData.version);
	std::cout << ", Contains " << std::to_string(metaData.numberOfInstructions) << " instructions, GMIDX [" << std::to_string(metaData.graphicsModeIndex) << "]" << std::endl;


	//24b instructions --> 3 bytes
	programCounter = 0u;
	run = true;
	for (unsigned int instructionIndex=0u; instructionIndex<metaData.numberOfInstructions; instructionIndex++) {
		unsigned int byteIndex = HEADER_LENGTH_BYTES + instructionIndex * 3;
		if (byteIndex+2u >= byteData.size()) {
			std::cout << "Tried to read past end of file. Exiting.." << std::endl;
			return false;
		}

		unsigned int instruction = (
			(static_cast<unsigned int>(byteData[byteIndex]) << 16u) |
			(static_cast<unsigned int>(byteData[byteIndex + 1u]) << 8u) |
			(static_cast<unsigned int>(byteData[byteIndex + 2u]))
		);

		instructionData->push_back(instruction);
	}

	return true;

}

}



namespace utils {

	static inline std::string strToUpper(const std::string& input) {
		std::string result = input;
		std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c){return std::toupper(c);});
		return result;
	}

}
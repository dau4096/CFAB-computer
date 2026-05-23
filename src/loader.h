#pragma once

#include "includes.h"
#include "constants.h"
#include "utils.h"
#include "instr.h"

using namespace std;


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
		(header[0u] == 0x43u) && //"C"
		(header[1u] == 0x46u) && //"F"
		(header[2u] == 0x41u) && //"A"
		(header[3u] == 0x42u)    //"B"
	);

	metaData.version = header[4u]; //5th byte
	bool versionValid = (metaData.version >= 2u); //Versions before 2 do not have a header.

	metaData.numberOfInstructions = (static_cast<uint16_t>(header[5u]) << 8u) | static_cast<uint16_t>(header[6u]); //Next 16 bits.
	bool numInstructionsValid = (
		(metaData.numberOfInstructions > 0u) &&
		(numberOfReadBytes >= (HEADER_LENGTH_BYTES+(metaData.numberOfInstructions*3u)))
	);

	metaData.graphicsModeIndex = (header[7u] & 0xC0u) >> 6u; //First 2 bits of this byte.
	graphicsMode = static_cast<GraphicsMode>(metaData.graphicsModeIndex);
	metaData.numberOfROMSegments = (static_cast<uint16_t>((header[7u]&0x3Fu)) << 4u) | ((static_cast<uint16_t>(header[8u]) & 0xF0u) >> 4u); //Next 10 bytes.

	return hasIdentifier && versionValid && numInstructionsValid;

}


bool loadInstructions(std::vector<uint8_t>& byteData, std::vector<Instruction>* instructionData) {
	//24b instructions --> 3 bytes
	programCounter = 0u;
	run = true;
	for (unsigned int instructionIndex=0u; instructionIndex<metaData.numberOfInstructions; instructionIndex++) {
		unsigned int byteIndex = HEADER_LENGTH_BYTES + instructionIndex * 3;
		if (byteIndex+2u >= byteData.size()) {
			std::cout << "Tried to read instructions past end of file." << std::endl;
			return false;
		}

		Instruction instruction = Instruction(
			(static_cast<unsigned int>(byteData[byteIndex]) << 16u) |
			(static_cast<unsigned int>(byteData[byteIndex + 1u]) << 8u) |
			(static_cast<unsigned int>(byteData[byteIndex + 2u]))
		);

		instructionData->push_back(instruction);
	}

	return true;
}


bool loadROMIndices(const std::vector<uint8_t>& byteData, size_t romIndexOffset) {
	readOnlyMemoryIndices.clear();
	readOnlyMemoryIndices.reserve(metaData.numberOfROMSegments + 1u);

	for (uint16_t ROMsegmentIndex=0u; ROMsegmentIndex<(metaData.numberOfROMSegments); ROMsegmentIndex++) {
		size_t idx = romIndexOffset + (ROMsegmentIndex * 2u);
		if ((idx+3u) >= byteData.size()) {
			std::cout << "Tried to read ROM segment indices past end of file." << std::endl;
			return false;
		}

		uint16_t start = (byteData[idx+0u] << 8u) | byteData[idx+1u];
		uint16_t end = (byteData[idx+2u] << 8u) | byteData[idx+3u];

		readOnlyMemoryIndices.emplace_back(start, end);

		if (verbose) {std::cout << "Segment " << std::to_string(ROMsegmentIndex) << ": [" << std::to_string(start) << " : " << std::to_string(end) << "]\n";}
	}
	std::cout<<std::flush;

	return true;
}



bool loadROMData(std::vector<uint8_t>& byteData, size_t romIndexOffset) {
	if (metaData.numberOfROMSegments < 1) {return true; /* No data to load - But did not "fail". */}
	size_t romDataOffset = romIndexOffset + ((metaData.numberOfROMSegments+1u) * 2u);
	if (romDataOffset >= byteData.size()) {
		std::cout << "Tried to read ROM segment data past end of file." << std::endl;
		return false;
	}

	readOnlyMemory.assign(byteData.begin() + romDataOffset, byteData.end());


	return true;
}


bool loadCFABFile(const std::string& filePath, std::vector<Instruction>* instructionData) {
	//Read file
	std::vector<uint8_t> byteData;
	if (!loadBytesData(filePath, byteData)) {
		std::cout << "Failed to read bytes from file." << std::endl;
		return false;
	}


	//Parse the header.
	bool validFile;
	if (byteData.size() >= HEADER_LENGTH_BYTES) {
		std::array<uint8_t, HEADER_LENGTH_BYTES> header;
		std::copy_n(byteData.begin(), HEADER_LENGTH_BYTES, header.begin());
		validFile = parseHeader(header, filePath, byteData.size());

	} else {validFile = false;}
	if (!validFile) {std::cout << "File header invalid. If this is an old .dat file, please re-fabricate with a newer fabricator version." << std::endl; return false;}

	//Print metadata;
	if (verbose) {
		std::cout << "\033[1;30m<METADATA [FilePath: [" << metaData.filePath << "], Version " << std::to_string(metaData.version);
		std::cout << ", Contains " << std::to_string(metaData.numberOfInstructions) << " instructions, GMIDX [" << std::to_string(metaData.graphicsModeIndex) << "]]>\033[0;m" << std::endl;
	}

	bool successINSTR = loadInstructions(byteData, instructionData);
	if (!successINSTR) {return false;}


	size_t numberOfInstructionBytes = HEADER_LENGTH_BYTES + metaData.numberOfInstructions * 3u;
	bool successROMIDX = loadROMIndices(byteData, numberOfInstructionBytes);
	if (!successROMIDX) {return false;}

	bool successROMDAT = loadROMData(byteData, numberOfInstructionBytes);
	if (!successROMDAT) {return false;}

	return true;

}

}
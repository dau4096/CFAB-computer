#pragma once

#include "includes.h"
#include "constants.h"
#include "utils.h"

using namespace std;



enum GraphicsMode {
	GM_NONE,
	GM_TEXT,
	GM_RGBc,
	GM_256c
};


inline GraphicsMode graphicsMode = GM_NONE;



namespace ByteColour { //256 colour mode - used by GM_RGBc and more directly by GM_256c

//Colour cubes.
const std::array<unsigned int, 6> cubeLevels = {
	0u, 95u, 135u, 175u, 215u, 255u
};


uint8_t quantizeChannel(unsigned int colourRGB) {
	uint8_t bestIndex = 0u;
	float bestDist = 1.0e7f;

	//Find closest cube.
	for (uint8_t i=0u; i<6; i++) {
		float d = glm::abs(colourRGB - cubeLevels[i]);
		if (d < bestDist) {
			bestDist = d;
			bestIndex = i;
		}
	}

	return bestIndex;
}


uint8_t rgbToXterm256(unsigned int uintR, unsigned int uintG, unsigned int uintB) {
	glm::uvec3 inputColor = glm::vec3(uintR, uintG, uintB);

	//Map onto the cube.
	uint8_t indexR = quantizeChannel(uintR);
	uint8_t IndexG = quantizeChannel(uintG);
	uint8_t indexB = quantizeChannel(uintB);

	glm::uvec3 cubeColor = glm::uvec3(
		cubeLevels[indexR],	cubeLevels[IndexG],	cubeLevels[indexB]
	);

	uint8_t cubeIndex = 16u + (36u*indexR) + (6u*IndexG) + indexB;

	glm::vec3 deltaRGB = inputColor - cubeColor;
	float cubeDistanceSq = glm::dot(deltaRGB, deltaRGB);


	//Map to greyscale steps
	//[8, 18, 28, .., 238] (24 levels, +10 each time.)

	float luminance = float(uintR + uintG + uintB) / 3.0f;
	uint8_t greyIndex = uint8_t(round((luminance - 8.0f) / 10.0f));
	greyIndex = glm::clamp(greyIndex, uint8_t(0u), uint8_t(23u)); //Clamp to a step.
	uint8_t greyValue = 8u + greyIndex * 10u;

	glm::uvec3 greyColor = glm::uvec3(greyValue, greyValue, greyValue);
	uint8_t greyCode = 232u + greyIndex;

	glm::vec3 deltaLUM = inputColor - greyColor;
	float greyDistanceSq = dot(deltaLUM, deltaLUM);


	//Choose closer colour;
	if (greyDistanceSq < cubeDistanceSq) {
		return greyCode;
	} else {
		return cubeIndex;
	}

}


uint8_t twoByteToIndex(uint16_t twoByte) {

	//Remap RGB16 
	unsigned int uintR = ((twoByte >> 10u) & 0x1Fu) << 3; //First 5 bits. Multiplied by 8
	unsigned int uintG = ((twoByte >> 5u) & 0x3Fu) << 2;  //Next 6 bits. Multiplied by 4
	unsigned int uintB = ((twoByte >> 0u) & 0x1Fu) << 3;  //Next 5 bits. Multiplied by 8

	return rgbToXterm256(uintR, uintG, uintB);

}




}


namespace graphics {


//COUT a coloured space char.
inline void COUTcolour(uint8_t colourIndex) {std::cout << "\033[48;5;" << colourIndex << "m \033[0m";}


void drawRGBc() {
	for (uint8_t y=0u; y<SCREEN_HEIGHT; y++) {
		for (uint8_t x=0; x<SCREEN_WIDTH; x++) {
			uint16_t RAMaddr = SCREEN_INDEX + (static_cast<uint16_t>(y) * SCREEN_WIDTH) + static_cast<uint16_t>(x);
			uint16_t memoryValue = randomAccessMemory[RAMaddr];
			uint8_t colourIndex = ByteColour::twoByteToIndex(memoryValue);
			COUTcolour(colourIndex);
		}
		std::cout << "\n"; //Newline without flush.
	}
	std::cout << std::flush;
}


void draw256c() {
	for (uint8_t y=0u; y<SCREEN_HEIGHT; y++) {
		for (uint8_t x=0; x<SCREEN_WIDTH; x++) {
			uint16_t RAMaddr = SCREEN_INDEX + (static_cast<uint16_t>(y) * SCREEN_WIDTH) + static_cast<uint16_t>(x);
			uint16_t memoryValue = randomAccessMemory[RAMaddr];
			COUTcolour(memoryValue & BITS_8);
		}
		std::cout << "\n"; //Newline without flush.
	}
	std::cout << std::flush;
}


void drawTEXT() { //GM_TEXT outputs ASCII characters to the terminal via the memory value.
	for (uint8_t y=0u; y<SCREEN_HEIGHT; y++) {
		for (uint8_t x=0; x<SCREEN_WIDTH; x++) {
			uint16_t RAMaddr = SCREEN_INDEX + (static_cast<uint16_t>(y) * SCREEN_WIDTH) + static_cast<uint16_t>(x);
			uint16_t memoryValue = randomAccessMemory[RAMaddr];
			std::cout << char(memoryValue);
		}
		std::cout << "\n"; //Newline without flush.
	}
	std::cout << std::flush;
}



void drawScreen() {

	switch (graphicsMode) {
		case GM_RGBc: { //Use sampling to find closest 256 colour equivalent.
			drawRGBc();
			break;
		}

		case GM_256c: { //Directly draw using this as index.
			draw256c();
			break;
		}

		case GM_TEXT: {
			drawTEXT();
			break;
		}

		case GM_NONE:
		default: {
			return;
		}
	}

}

}
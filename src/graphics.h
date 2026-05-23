#pragma once

#include "includes.h"
#include "constants.h"
#include "utils.h"

using namespace std;





namespace ANSI256 { //256 colour mode - Used for GM_256c.

//Colour cubes.
const std::array<unsigned int, 6> cubeLevels = {
	0u, 95u, 135u, 175u, 215u, 255u
};


uint8_t quantizeChannel(uint8_t channelV) {
	if (channelV < 48u) {return 0u;}
	if (channelV < 114u) {return 1u;}
	return (channelV - 35u) / 40u;
}


uint8_t rgbToXterm256(uint8_t R, uint8_t G, uint8_t B) {
	if ((R == G) && (G == B)) { //Greyscale
		uint8_t greyIndex = (R - 8 + 5) / 10;
		greyIndex = glm::clamp(greyIndex, uint8_t(0), uint8_t(23));
		return 232u + greyIndex;
	}

	uint8_t Rquant = quantizeChannel(R);
	uint8_t Gquant = quantizeChannel(G);
	uint8_t Bquant = quantizeChannel(B);

	return 16u + (36u * Rquant) + (6u * Gquant) + Bquant;
}


inline void appendNumber(std::string &s, uint8_t n) {
	//Quick 8b to string
	if (n >= 100) {
		s += '0' + n / 100;
		n %= 100;
		s += '0' + n / 10;
		n %= 10;
		s += '0' + n;
	} else if (n >= 10) {
		s += '0' + n / 10;
		n %= 10;
		s += '0' + n;
	} else {
		s += '0' + n;
	}
}


void convertRGB332toXTerm(std::vector<int8_t>* RAMdata, std::vector<uint8_t>* xTermData) {
	//Convert from 8-bit RRRGGGBB to 256 colour xTerms
	for (unsigned int y=0u; y<SCREEN_HEIGHT; y++) {
		for (unsigned int x=0u; x<SCREEN_WIDTH; x++) {
			unsigned int index = (y*SCREEN_WIDTH)+x;
			uint8_t RAMvalue = (uint8_t)(RAMdata->at(index));

			//Seperate into R, G and B.
			//All occupy the first n bits.
			uint8_t R = (RAMvalue << 0u) & 0xE0u; //First 3 bits
			uint8_t G = (RAMvalue << 3u) & 0xE0u; //Middle 3
			uint8_t B = (RAMvalue << 5u) & 0xC0u; //2 final bits

			std::cout << R << " " << G << " " << B << std::endl;
			uint8_t xTerm = rgbToXterm256(R, G, B); //Convert
			xTermData->at(index) = xTerm;
		}
	}
}







//SCREEN_DOUBLE_SCALE Changes the screen to draw 2 spaces rather than ½ square characters. Quadruples screen size.
#ifndef SCREEN_DOUBLE_SCALE //NOT defined, small pixels.
void drawScreenColours(std::vector<uint8_t>& xTermData) {
	//Move cursor to top-left and disable wraparound.
	std::string term256;
	term256.reserve((SCREEN_ELEMENT_SIZE * 12u)); //Estimate.
	term256 += "\x1b[H\x1b[2J\x1b[3J\x1b[?7l";

	int lastFG = -1; int lastBG = -1;


	unsigned int consoleHeight = SCREEN_HEIGHT / 2u;
	unsigned int consoleWidth = glm::min(SCREEN_WIDTH, static_cast<unsigned int>(consoleResolution.x));

	for (unsigned int y=consoleHeight; y>0u; y--) {
		unsigned int topBase = ((y-1u) * 2u) * SCREEN_WIDTH;
		unsigned int lowBase = topBase + SCREEN_WIDTH;

		for (unsigned int x=0u; x<consoleWidth; x++) {
			unsigned int topPixel = topBase + x;
			unsigned int lowPixel = lowBase + x;

			uint8_t top = xTermData.at(topPixel);
			uint8_t low = xTermData.at(lowPixel);

			//Only cout SGR if colour changed
			if (top != lastBG) { //Background, upper PX.
				term256 += "\x1b[48;5;";
				appendNumber(term256, top);
				term256 += "m";
				lastBG = top;
			}
			if (low != lastFG) { //Foreground, lower PX.
				term256 += "\x1b[38;5;";
				appendNumber(term256, low);
				term256 += "m";
				lastFG = low;
			}

			term256 += "▀"; //UTF half-block char.
		}

		term256 += '\n';
		lastFG = lastBG = -1; //Reset after each line
	}

	//Reset formatting, output.
	term256 += "\x1b[?7h\x1b[0m";
	fwrite(term256.data(), 1, term256.size(), stdout);
	fflush(stdout);	
}

#else //IS defined, large pixels.
void drawScreenColours(std::vector<uint8_t>& xTermData) {
	//Move cursor to top-left and disable wraparound.
	std::string term256;
	term256.reserve((SCREEN_ELEMENT_SIZE * 12u)); //Estimate.
	term256 += "\x1b[H\x1b[2J\x1b[3J\x1b[?7l";
	int lastColour = -1;

	unsigned int consoleWidth = glm::min(SCREEN_WIDTH, static_cast<unsigned int>(consoleResolution.x));
	for (unsigned int y=SCREEN_HEIGHT; y>0u; y--) {
		for (unsigned int x=0u; x<consoleWidth; x++) {
			unsigned int index = ((y-1u) * SCREEN_WIDTH) + x;
			uint8_t xTerm = xTermData.at(index);
			if (xTerm != lastColour) {
				term256 += "\x1b[48;5;"; //Add colour to background.
				appendNumber(term256, xTerm);
				term256 += "m";
				lastColour = xTerm;
			}
			term256 += "  "; //2 spaces.
		}
		term256 += '\n';
		lastColour = -1; //Reset after each line
	}

	//Reset formatting, output.
	term256 += "\x1b[?7h\x1b[0m";
	fwrite(term256.data(), 1, term256.size(), stdout);
	fflush(stdout);	
}
#endif






void drawScreenText(std::vector<uint8_t>& UTFdata) {
	//TBA.
}


void copyScreenDataIntoVector(std::vector<int8_t>* RAMdata) {
	RAMdata->reserve(SCREEN_ELEMENT_SIZE);
	std::copy_n(
		randomAccessMemory + SCREEN_START_INDEX,
		SCREEN_ELEMENT_SIZE, RAMdata->begin() //Copy values from RAM into the RAMdata vector.
	);
}



}


namespace graphics {

void drawCurrentScreen() {
	//Read screen from RAM, and display if needed.
	std::cout << "graphicsMode: GM_";
	if (graphicsMode == GM_NONE) {std::cout << "NONE" << std::endl; return; /* Don't check ahead, no graphics. */}

	consoleResolution = utils::getConsoleResolution();

	std::vector<int8_t> RAMdata(SCREEN_ELEMENT_SIZE);
	ANSI256::copyScreenDataIntoVector(&RAMdata);


	switch (graphicsMode) {
		case GM_TEXT: { //Use RAM values as unicode symbols.
			std::cout << "TEXT" << std::endl;
			ANSI256::drawScreenText(reinterpret_cast<std::vector<uint8_t>&>(RAMdata)); break;
		}
		case GM_256c: { //Directly use values from RAM as colours to be drawn.
			std::cout << "256c" << std::endl;
			ANSI256::drawScreenColours(reinterpret_cast<std::vector<uint8_t>&>(RAMdata)); break;
		}
		case GM_RGBc: { //Convert values from RAM into RGB Xterms.
			std::cout << "RGBc" << std::endl;
			//The 8 bits of each value is divided like so;
			//RRRGGGBB
			std::vector<uint8_t> xTermData(SCREEN_ELEMENT_SIZE);
			ANSI256::convertRGB332toXTerm(&RAMdata, &xTermData);
			ANSI256::drawScreenColours(xTermData);
			break;
		}
		default: {break;}
	}

	std::cout << std::to_string(randomAccessMemory[0xE00]) << " ";       //0101 1111
	std::cout << std::to_string(randomAccessMemory[0xE01]) << " ";       //1001 1100
	std::cout << std::to_string(randomAccessMemory[0xE02]) << std::endl; //1000 0011

	//std::cout << std::to_string(randomAccessMemory[0xE00]) << " ";
	//std::cout << std::to_string(randomAccessMemory[0xE00]) << std::endl;
}

}
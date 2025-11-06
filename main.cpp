#include "src/includes.h"
#include "src/constants.h"
#include "src/utils.h"
#include "src/instr.h"
#include "src/tests.h"
#include "cxxopts.hpp"
using namespace std;


void debug() {
	std::cout << "\033[1;30m-DEBUG-\033[0;m" << std::endl;
	std::vector<unsigned int> debugInstructions = {
		0x410001u, //SET r0 to #1
		0x410102u, //SET r1 to #2
		0x030001u, //ADD r0 to r1
	};
	CFAB::runInstructionSet(debugInstructions);	
}





std::string filePath = FILE_PATH;
bool enablePeek = false, runNormal = true;

void handleArguments(int argc, char* argv[]) {
	//Process arguments
	cxxopts::Options options = cxxopts::Options("CFAB-Computer/V2", "Processes fabricated CFABv2 instructions.");

    options.add_options()
        ("f,file", "Datafile path", cxxopts::value<std::string>())
        ("p,peek", "Peek memory addresses after exec.")
        ("v,verbose", "Show detailed output from commands.")
        ("r,rate", "Show elapsed time and frequency.")
        ("t,test", "Run automated tests.")
        ("d,debug", "Minimal debugging setup.");

    auto result = options.parse(argc, argv);


    verbose = result.count("verbose");
    if (result.count("file")) {
    	filePath = result["file"].as<std::string>();
    }
    if (result.count("peek")) {
    	enablePeek = true;
    }
    if (result.count("test")) {
		std::cout << "\033[1;30m-TESTS-\033[0;m" << std::endl;
		tests::doTests();
        runNormal = false;
    }
    if (result.count("debug")) {
        debug();
        runNormal = false;
    }
    if (result.count("rate")) {
    	checkSpeed = true;
    }

}



int main(int argc, char* argv[]) {
	std::cout << "\033[1;33m[START]\033[0;m" << std::endl;
	handleArguments(argc, argv);


	if (runNormal) {

		std::cout << "\033[1;30m-NORMAL-\033[0;m" << std::endl;
		std::vector<unsigned int> instructionData;
		if (!loader::loadInstructions(filePath, &instructionData)) {
			std::cerr << "\033[1;31mFailed to load instructions from: " << FILE_PATH << "\033[0;m" << std::endl;
			return -1;
		}
		std::cout << "\033[1;30mLoaded " << instructionData.size() << " instructions from [" << filePath << "]\033[0;m" << std::endl;


		//Execute.
		CFAB::runInstructionSet(instructionData);
	}

	std::cout << "\033[1;33m[END]\033[0;m" << std::endl;

	if (enablePeek) {
		//User requests to view specific register indices.
		std::cout << std::endl << "> ";
		std::string type, indexStr;
		std::cin >> type >> indexStr;
		while (1) {
			int index = std::stoi(indexStr);
			if (type == "r") {
				std::cout << "r" << index << ": " << std::to_string(registers[index]) << std::endl;
			} else if (type == "ram") {
				std::cout << "RAM-" << index << ": " << std::to_string(randomAccessMemory[index]) << std::endl;
			}
			std::cin >> type >> indexStr;
		}
	}
}
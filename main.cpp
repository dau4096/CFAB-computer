#include "src/includes.h"
#include "src/constants.h"
#include "src/utils.h"
#include "src/instr.h"
#include "src/tests.h"
#include "cxxopts.hpp"
using namespace std;


void debug() {
	std::cout << "-DEBUG-" << std::endl;
	std::vector<unsigned int> debugInstructions = {
		0x410001u, //SET r0 to #1
		0x410102u, //SET r1 to #2
		0x030001u, //ADD r0 to r1
	};
	CFAB::runInstructionSet(debugInstructions);	
}





std::string filePath = FILE_PATH;
bool enablePeek = false, runNormal = true, checkSpeed = false;

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
		std::cout << "-TESTS-" << std::endl;
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
	std::cout << "START" << std::endl;
	handleArguments(argc, argv);


	if (runNormal) {

		std::cout << "-NORMAL-" << std::endl;
		std::vector<unsigned int> instructionData;
		if (!loader::loadInstructions(filePath, &instructionData)) {
			std::cerr << "Failed to load instructions from: " << FILE_PATH << std::endl;
			return -1;
		}
		std::cout << "Loaded " << instructionData.size() << " instructions from [" << filePath << "]" << std::endl;


		//Execute.
		std::chrono::time_point<std::chrono::high_resolution_clock> start;
		if (checkSpeed) {
			start = std::chrono::high_resolution_clock::now();
		}

		CFAB::runInstructionSet(instructionData);

		if (checkSpeed) {
			std::chrono::time_point<std::chrono::high_resolution_clock> end = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double, std::milli> elapsed = end - start;
			double freq = static_cast<double>(numExecuted)*1000.0f/elapsed.count();
			std::string freqStr = (freq > 1.0e3f) ? std::to_string(freq / 1000.0f)+"k" : std::to_string(freq);

			std::cout << std::endl;
			std::cout << "Executed:  \033[1;35m" << std::to_string(numExecuted) << " instructions\033[0;m" << std::endl;
			std::cout << "Elapsed:   \033[1;35m" << elapsed.count() << "ms\033[0;m" << std::endl;
			std::cout << "Frequency: \033[1;35m" << freqStr << "Hz\033[0;m" << std::endl;
			std::cout << std::endl;
		}
	}

	std::cout << "END" << std::endl;

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
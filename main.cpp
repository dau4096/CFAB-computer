#include "src/includes.h"
#include "src/constants.h"
#include "src/utils.h"
#include "src/instr.h"
#include "src/tests.h"
using namespace std;








int main(int argc, char* argv[]) {

	std::cout << "START" << std::endl;


#ifdef DEBUG

	#ifdef DEBUG_RUN_TESTS

		std::cout << "-TESTS-" << std::endl;
		tests::run();

	#else

		std::cout << "-DEBUG-" << std::endl;
		std::vector<unsigned int> debugInstructions = {
			0x410001u, //SET r0 to #1
			0x410102u, //SET r1 to #2
			0x030001u, //ADD r0 to r1
		};
		CFAB::runInstructionSet(debugInstructions);

	#endif


#else

	std::cout << "-NORMAL-" << std::endl;
	std::vector<unsigned int> instructionData;
	std::string filePath = (argc > 1) ? argv[1] : FILE_PATH;
	if (!loader::loadInstructions(filePath, &instructionData)) {
		std::cerr << "Failed to load instructions from: " << FILE_PATH << std::endl;
		return -1;
	}
	std::cout << "Loaded " << instructionData.size() << " instructions" << std::endl;

	//Exec.
	CFAB::runInstructionSet(instructionData);

#endif


	std::cout << "END" << std::endl;

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
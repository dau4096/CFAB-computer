#include "src/includes.h"
#include "src/constants.h"
#include "src/utils.h"
#include "src/instr.h"
#include "src/tests.h"
using namespace std;








int main() {

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
	if !(loader::loadInstructions(FILE_PATH, &instructionData)) {
		std::cerr << "Failed to load instructions from: " << FILE_PATH << std::endl;
		return -1;
	}
	//Exec.
	CFAB::runInstructionSet(instructionData);

#endif


	std::cout << "END" << std::endl;

}
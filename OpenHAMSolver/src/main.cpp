/*
OpenHAM solver

Copyright Andreas Nicolai
Published under the GNU GPL, see LICENSE document.

*/

#include <iostream>
#include <cstdlib>
#include <string>

#include "Integrator.h"
#include "Model.h"

int main(int argc, char * argv[]) {

	if (argc < 2) {
		std::cerr << "Missing project file argument." << std::endl;
		return EXIT_FAILURE;
	}

	std::string projectFile = argv[1];
	if (!IBK::Path(projectFile).exists()) {
		std::cerr << "Project file '"<< projectFile << "' does not exist." << std::endl;
		return EXIT_FAILURE;
	}

	Integrator integrator;
	Model & model = integrator.m_model;

	try {
		// setup model
		model.init(projectFile);
	}
	catch (IBK::Exception & ex) {
		ex.writeMsgStackToError();
		std::cerr << "Error during initialization." << std::endl;
		return EXIT_FAILURE;
	}

	try {
		// run integrator
		integrator.run();
	}
	catch (std::exception & ex) {
		std::cerr << ex.what() << std::endl;
		std::cerr << "Error during solver run." << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}


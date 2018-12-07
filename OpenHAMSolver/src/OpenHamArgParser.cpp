#include "OpenHamArgParser.h"

OpenHAMArgParser::OpenHAMArgParser() {
	addFlag(0, "no-disc", "Skip grid generation (use grid as written in project file)");
	addOption(0, "disc", "Custom grid generation, either equidistant (within each layer) or variable grid.", "equi|var>:<min dx>[:<stretch>]", "");
}

void OpenHAMArgParser::printHelp(std::ostream & out) const {
	// re-use implementation of original help function
	out << "Syntax: OpenHAMSolver [options] <project file>\n\n";
	const unsigned int TEXT_WIDTH = 79;
	const unsigned int TAB_WIDTH = 26;
	printFlags(out,TEXT_WIDTH,TAB_WIDTH);
	printOptions(out,TEXT_WIDTH,TAB_WIDTH);
	out << "\nExamples:\n\n"
		<< "Generating an equi-distant grid with 2 mm grid cells:\n"
		<< "> OpenHamSolver --disc=equi:0.002 project.d6p\n\n"
		<< "Generating a grid with variable spacing with 1 mm grid cells at boundaries and stretch factor of about 1.3:\n"
		<< "> OpenHamSolver --disc=var:0.001:1.3 project.d6p\n\n"
		<< "Detailed progress output and writing statistics after each completed simulation step:\n"
		<< "> OpenHamSolver --verbosity-level=3 --step-stats project.d6p\n\n";
}


void OpenHAMArgParser::printManPage(std::ostream & out) const {
	IBK::ArgParser::printManPage(out);
}

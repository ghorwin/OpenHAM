/*	Copyright (c) 2001-today, Institut für Bauklimatik, TU Dresden, Germany

	Written by Andreas Nicolai
	All rights reserved.

	This file is part of the OpenHAM software.

	Redistribution and use in source and binary forms, with or without modification,
	are permitted provided that the following conditions are met:

	1. Redistributions of source code must retain the above copyright notice, this
	   list of conditions and the following disclaimer.

	2. Redistributions in binary form must reproduce the above copyright notice,
	   this list of conditions and the following disclaimer in the documentation
	   and/or other materials provided with the distribution.

	3. Neither the name of the copyright holder nor the names of its contributors
	   may be used to endorse or promote products derived from this software without
	   specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
	ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
	ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "OpenHAMArgParser.h"

OpenHAMArgParser::OpenHAMArgParser() {
	addFlag(0, "no-disc", "Skip grid generation (use grid as written in project file)");
	addOption(0, "disc", "Custom grid generation, either equidistant (within each layer) or variable grid.", "equi|var>:<min dx>[:<stretch>]", "");
	m_appname = "OpenHAMSolver";
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


void OpenHAMArgParser::extractDiscretizationOptions(bool & variableDisc, double & dx, double & stretch) const {
	const char * const FUNC_ID = "[Model::extractDiscretizationOptions]";
	// extract discretization options
	if (static_cast<IBK::ArgParser>(*this).hasOption("disc")) {
		if (static_cast<IBK::ArgParser>(*this).flagEnabled("no-disc"))
			throw IBK::Exception("You must not use 'no-disc' and 'disc' command line options at the same time.", FUNC_ID);

		// explode argument
		std::vector<std::string> tokens;
		IBK::explode(static_cast<IBK::ArgParser>(*this).option("disc"), tokens, ':', true);

		// must have at least 2 tokens
		if (tokens.size() < 2)
			throw IBK::Exception("Invalid argument to 'disc' command line option.", FUNC_ID);

		// second token is always dx
		try {
			dx = IBK::string2val<double>(tokens[1]);
			if (dx <= 0)
				throw IBK::Exception("Invalid dx value in 'disc' command line option.", FUNC_ID);
		}
		catch (...) {
			throw IBK::Exception("Invalid dx value in 'disc' command line option.", FUNC_ID);
		}

		// operation
		if (IBK::string_nocase_compare(tokens[0], "equi")) {
			variableDisc = false;
		}
		else if (IBK::string_nocase_compare(tokens[0], "var")) {
			variableDisc = true;

			// third token is stretch
			if (tokens.size() != 3)
				throw IBK::Exception("Invalid argument to 'disc' command line option.", FUNC_ID);
			try {
				stretch = IBK::string2val<double>(tokens[2]);
				if (stretch <= 1)
					throw IBK::Exception("Invalid stretch factor in 'disc' command line option.", FUNC_ID);
			}
			catch (...) {
				throw IBK::Exception("Invalid stretch factor in 'disc' command line option.", FUNC_ID);
			}

		}
		else
			throw IBK::Exception("Invalid operation in 'disc' command line option.", FUNC_ID);
	}
}


/*	Copyright (c) 2001-2017, Institut für Bauklimatik, TU Dresden, Germany

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

#include <iostream>
#include <cstdlib>
#include <string>

#include <IBK_MessageHandler.h>
#include <IBK_Exception.h>
#include <IBK_messages.h>
#include <IBK_StringUtils.h>
#include <IBK_WaitOnExit.h>
#include <IBK_SolverArgsParser.h>

#include "Integrator.h"
#include "Model.h"

/// Version info string
const char * const PROGRAM_INFO =
	"OpenHAM Solver, Version 0.3\n"
	"All rights reserved. Distributed under GPL v3.\n\n"
	"Copyright 2015-today Andreas Nicolai\n"
	"Contact: andreas.nicolai [at] tu-dresden.de\n\n";

// Function sets up output directory structure
void setupDirectories(const IBK::SolverArgsParser & args, Model & model);


/*! Solver main() function. */
int main(int argc, char * argv[]) {
	const char * const FUNC_ID = "[main]";

	IBK::WaitOnExit wait_on_exit(true);

	try {
		// enable utf8 string output on windows
		IBK::MessageHandler::setupUtf8Console();

		// *** Command line parsing ***

		IBK::SolverArgsParser args;
		args.setAppName("OpenHAMSolver");

		// Note: Command line arguments are passed as char in local encoding, i.e. in german latin9 encoding
		//       They are converted to UTF8 (on Windows) for the rest of the solver to work.
		args.parse(argc, argv);
		wait_on_exit.m_wait = !args.flagEnabled(IBK::SolverArgsParser::DO_CLOSE_ON_EXIT);
		// handle default arguments like help and man-page requests, which are printed to std::cout
		if (static_cast<IBK::ArgParser>(args).flagEnabled("help")) {
			// remove arguments not supported by OpenHAMSolver so that they do not show up in help
			args.removeOption("disable-headers");
			args.removeOption("restart");
			args.removeOption("restart-from");
			args.removeOption("test-init");
			args.removeOption("output-dir");
			args.removeOption("parallel-threads");
			args.removeOption("integrator");
			args.removeOption("les-solver");
			args.removeOption("precond");
		}
		if (args.handleDefaultFlags(std::cout))
			// stop if help/man-page requested
			return EXIT_SUCCESS;

		// print out version if requested
		if (args.flagEnabled(IBK::SolverArgsParser::DO_VERSION)) {
			std::cout << PROGRAM_INFO << std::endl;
			return EXIT_SUCCESS;
		}

		// check if errors are present, error messages are written to std::cerr
		if (args.handleErrors(std::cerr))
			return EXIT_FAILURE;

		// *** create integrator ***
		Integrator integrator;
		Model & model = integrator.m_model; // 'model' is just a readability alias

		// *** create directory structure ***
		setupDirectories(args, model);
		// now we have a log directory and can write our messages to the log file

		// *** setup message handler ***

		std::string logfile = model.m_dirs.m_logDir.str() + "/screenlog.txt";
		int verbosityLevel = IBK::string2val<int>(args.option(IBK::SolverArgsParser::DO_VERBOSITY_LEVEL));
		IBK::MessageHandler * messageHandlerPtr = dynamic_cast<IBK::MessageHandler *>(IBK::MessageHandlerRegistry::instance().messageHandler());
		messageHandlerPtr->setConsoleVerbosityLevel(verbosityLevel);
		messageHandlerPtr->m_contextIndentation = 48;
		std::string errmsg;
		bool success = messageHandlerPtr->openLogFile(logfile, args.m_restart, errmsg);
		if (!success) {
			IBK::IBK_Message(errmsg, IBK::MSG_WARNING, FUNC_ID);
			IBK::IBK_Message("Cannot create log file, outputs will only be printed on screen.", IBK::MSG_WARNING, FUNC_ID);
		}

		// *** write program/copyright info ***
		IBK::IBK_Message(PROGRAM_INFO, IBK::MSG_PROGRESS, FUNC_ID, IBK::VL_STANDARD);
		IBK::IBK_Message("\n", IBK::MSG_PROGRESS, FUNC_ID, IBK::VL_STANDARD);


		// *** initialize model ***
		model.init(args);

		// *** run solver ***
		integrator.m_solverStepStats = args.flagEnabled(IBK::SolverArgsParser::DO_STEP_STATS);
		IBK::IBK_Message("Initialization complete, starting solver.\n", IBK::MSG_PROGRESS, FUNC_ID, IBK::VL_STANDARD);
		integrator.run();
	}
	catch (IBK::Exception & ex) {
		ex.writeMsgStackToError();
		IBK::IBK_Message("Critical error, simulation aborted.", IBK::MSG_ERROR, FUNC_ID);
		return EXIT_FAILURE;
	}
	catch (std::exception& ex) {
		IBK::IBK_Message(ex.what(), IBK::MSG_ERROR, FUNC_ID);
		IBK::IBK_Message("Critical error, simulation aborted.", IBK::MSG_ERROR, FUNC_ID);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}


/*! Utility function to setup output directory structure based on project file path and command line arguments. */
void setupDirectories(const IBK::SolverArgsParser & args, Model & model) {
	const char * const FUNC_ID = "[setupDirectories]";

	IBK::Path projectFile = args.m_projectFile;
	try {

		// ***** Check if project file exists *****
		if (!projectFile.isFile())
			throw IBK::Exception( IBK::FormatString("Project file '%1' does not exist (or access denied).").arg(projectFile), FUNC_ID);

		// ***** Create directory structure for solver log and output files *****
		IBK::Path projectPath = projectFile.parentPath();
		IBK::Path pfPath = projectFile.filename();

		// relative path argument without leading ./
		if (projectPath.str().empty())
			projectPath = "./";

		if (args.hasOption(IBK::SolverArgsParser::GO_OUTPUT_DIR)) {
			std::string outputPath = args.option(IBK::SolverArgsParser::GO_OUTPUT_DIR);
			IBK::trim(outputPath);
			if (outputPath.empty())
				throw IBK::Exception( "Empty output path specified with -o option.", FUNC_ID);
			model.m_dirs.create(projectPath.str(), outputPath);
		}
		else {
			std::string projectname_base = pfPath.withoutExtension().str();
			model.m_dirs.create(projectPath.str(), projectname_base);
		}

		// now our directory structure is available
	}
	catch (IBK::Exception & ex) {
		throw IBK::Exception(ex, "Error setting up directory structure for project '"+projectFile.str() + "'", FUNC_ID);
	}
	catch (std::exception & ex) {
		throw IBK::Exception("Error setting up directory structure for project '"+projectFile.str() +
							 "'. Failed with error message:\n" + ex.what(), FUNC_ID);
	}
}
// ---------------------------------------------------------------------------


/*!

\file main.cpp
\brief Implementation file with solver's main() function.

\mainpage About OpenHAM solver

An open source transport model implementation for heat, air and moisture transport in porous media. It is a
console-based, cross-platform solver with bindings to pre- and postprocessing software, meant as reference
implementation for the EN 15026 standard.

The physical model is implemented in class \a Model, the numerical integrator in class \a Integrator.

*/

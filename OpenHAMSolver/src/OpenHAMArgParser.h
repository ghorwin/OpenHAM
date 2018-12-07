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

#ifndef OpenHAMArgParserH
#define OpenHAMArgParserH

#include <IBK_SolverArgsParser.h>

/*! Command line parser class, extends the generic solver args parser class with custom
	functionality (actually removes arguments not supported by OpenHAM.
*/
class OpenHAMArgParser : public IBK::SolverArgsParser {
public:
	/*! Constructor. */
	OpenHAMArgParser();

	/*! Prints help page to std-output.
		\param out	Stream buffer of the help output.
	*/
	virtual void printHelp(std::ostream & out) const;

	/*! Prints man page to std-output.
		\param out	Stream buffer of the manual output.
	*/
	virtual void printManPage(std::ostream & out) const;

	/*! Utility function to parse command line arguments related to discretization. */
	void extractDiscretizationOptions(bool & variableDisc, double & dx, double & stretch) const;
};

#endif // OpenHAMArgParserH

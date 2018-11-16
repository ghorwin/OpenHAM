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

#include "Directories.h"

#include <cstdlib>
#include <functional>
#include <ctime>

#include <IBK_FormatString.h>
#include <IBK_FileUtils.h>
#include <IBK_Exception.h>
#include <IBK_Path.h>

void Directories::create(const std::string & projectpath, const std::string & projectfile) {
	const char * const FUNC_ID = "[Directories::create]";
	m_rootDir.clear();
	m_logDir.clear();
	m_varDir.clear();
	m_resultsDir.clear();

	// compose filename of project root folder
	m_projectDir = projectpath;
	std::string project_root = projectpath + '/' + projectfile;
	// if user specified alternative output path, 'projectFile' may be also an absolute path, in this case, set this
	// absolute path as project_root
	if (projectfile[0] == '/') {
		project_root = projectfile; // unix-style root-based path or network path
	}
	else if (projectfile[1] == ':') {
		project_root = projectfile; // drive letter detected, windows-style root-based path
	}

	// create solver base directory
	IBK::Path projectRootPath(project_root);
	if ( !projectRootPath.exists() ) {
		if (!IBK::Path::makePath(projectRootPath)) {
			throw IBK::Exception("Cannot create directory '" + project_root + "'. Perhaps missing priviliges?", FUNC_ID);
		}
	}

	IBK::Path logDirPath = projectRootPath;
	logDirPath.addPath( IBK::Path("/log") ); // Note: modifies logDirPath
	if (!logDirPath.exists()) {
		if (!IBK::Path::makePath(logDirPath)) {
			throw IBK::Exception("Cannot create directory '" + logDirPath.str() + "'. Perhaps missing priviliges?", FUNC_ID);
		}
	}

	IBK::Path vardirPath = projectRootPath;
	vardirPath.addPath( IBK::Path("/var") );
	if ( !vardirPath.exists() ) {
		if (!IBK::Path::makePath( vardirPath ) ) {
			throw IBK::Exception("Cannot create directory '" + vardirPath.str() + "'. Perhaps missing priviliges?", FUNC_ID);
		}
	}

	IBK::Path outdirPath = projectRootPath;
	outdirPath.addPath( IBK::Path("/results") );
	if (!outdirPath.exists()) {
		if ( !IBK::Path::makePath( outdirPath ) ) {
			throw IBK::Exception("Cannot create directory '" + outdirPath.str() + "'. Perhaps missing priviliges?", FUNC_ID);
		}
	}
	m_rootDir = projectRootPath;
	m_logDir = logDirPath;
	m_varDir = vardirPath;
	m_resultsDir = outdirPath;
}


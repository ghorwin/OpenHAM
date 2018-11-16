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

#ifndef OutputsH
#define OutputsH

#include "Model.h"

#include <DATAIO_DataIO.h>
#include <IBK_UnitVector.h>


/*! Handles outputs from model. */
class Outputs {
public:
	/*! Constructor, needs model to work. */
	Outputs(const Model * model);

	/*! Destructor, cleans up output file streams. */
	~Outputs();

	/*! Creates output files. */
	void setupOutputFiles(const IBK::Path & outputRootDir);

	/*! Appends outputs using current state of model. */
	void appendOutputs();

	/*! Writes geometry file. */
	void writeGeofile();

	/*! Adds a profile output. */
	void addProfileOutput(const std::string & fname,
						  const std::string & quantity,
						  const std::string & valueUnit);


	/*! Holds pointer to model instance (not owned). */
	const Model						*m_model;

	/*! Path to the ..../projectname/results directory. */
	IBK::Path						m_outputRootPath;

	/*! File name of geometry file. */
	std::string						m_geoFileName;
	/*! Geometry file hash. */
	unsigned int					m_geoFileHash;

	/*! DataIO containers. */
	std::vector<DATAIO::DataIO*>	m_dataIOs;

	/*! Output file stream for scalar outputs. */
	std::ostream					*m_surfaceValues;

	/*! Workspace vector for converting outputs and writing profiles. */
	IBK::UnitVector					m_profileVector;
};

#endif // OutputsH

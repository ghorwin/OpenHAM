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

#include "Outputs.h"

#include <DataIO>
#include <IBK_FileUtils.h>
#include <IBK_crypt.h>

Outputs::Outputs(const Model * model) :
	m_model(model),
	m_surfaceValues(NULL),
	m_integralValues(NULL)
{
}


Outputs::~Outputs() {
	delete m_surfaceValues;
	delete m_integralValues;
	for (unsigned int i=0; i<m_dataIOs.size(); ++i)
		delete m_dataIOs[i];
	for (unsigned int i=0; i<m_layerOutputs.size(); ++i)
		delete m_layerOutputs[i].outputStream;
}


void Outputs::setupOutputFiles(const IBK::Path & outputRootPath) {
	const char * const FUNC_ID = "[Outputs::setupOutputFiles]";

	try {

		// create output directory if it does not exist yet
		if (!outputRootPath.exists())
			IBK::Path::makePath(outputRootPath);

		m_outputRootPath = outputRootPath;

		// populate geoFile with data and write geometry file
		writeGeofile();

		// create output files
		// we need one file for scalar quantities:
		// - surface conditions on either side: T, rh, pv
		// - surface fluxes on either side: q, j_v, j_w, h_v, h_w

		// open file
		IBK::Path surfValFileName(m_outputRootPath / "SurfaceValues.tsv");
	#if defined(_WIN32)
		#if defined(_MSC_VER)
			m_surfaceValues = new std::ofstream( surfValFileName.wstr().c_str(), std::ios_base::binary);
		#else
			std::string filenameAnsi = IBK::WstringToANSI(surfValFileName.wstr(), false);
			m_surfaceValues = new std::ofstream( filenameAnsi.c_str(), std::ios_base::binary);
		#endif
	#else
		m_surfaceValues = new std::ofstream( surfValFileName.c_str() );
	#endif

		// compose header
		*m_surfaceValues << "Time [d]\t"
			 << "T_left [C]\t" << "RH_left [%]\t" << "pv_left [Pa]\t"
			 << "q_left [W/m2]\t" << "gv_left [kg/m2s]\t" << "gw_left [kg/m2s]\t" << "hv_left [W/m2]\t"<< "hw_left [W/m2]\t"
			 << "T_right [C]\t" << "RH_right [%]\t" << "pv_right [Pa]\t"
			 << "q_right [W/m2]\t" << "gv_right [kg/m2s]\t" << "hv_right [W/m2]" << std::endl;



		// open file
		IBK::Path integralValFileName(m_outputRootPath / "Integrals.tsv");
	#if defined(_WIN32)
		#if defined(_MSC_VER)
			m_integralValues = new std::ofstream( integralValFileName.wstr().c_str(), std::ios_base::binary);
		#else
			std::string filenameAnsi = IBK::WstringToANSI(integralValFileName.wstr(), false);
			m_integralValues = new std::ofstream( filenameAnsi.c_str(), std::ios_base::binary);
		#endif
	#else
		m_integralValues = new std::ofstream( integralValFileName.c_str() );
	#endif

		// compose header
		*m_integralValues << "Time [d]\t"
			 << "mwv [kg/m2]" << std::endl;

		// create profile outputs for: T, rh, pv, pvsat, w, w_hyg, w_ovhg

		// index 0 - temperature profile
		addProfileOutput("Profile_Temperature", "Temperature", "C");
		// index 1 - temperature profile
		addProfileOutput("Profile_RelativeHumidity", "Relative Humidity", "%");
		// index 2 - moisture content profile
		addProfileOutput("Profile_MoistureMassDensity", "Moisture Mass Density", "kg/m3");
		// index 3 - vapor pressure profile
		addProfileOutput("Profile_VaporPressure", "Water Vapor Pressure", "Pa");
//#define WRITE_TRANSPORT_FUNCTIONS
#ifdef WRITE_TRANSPORT_FUNCTIONS
		// index 4 - thermal conductivity profile
		addProfileOutput("Profile_Lambda", "Thermal Conductivity", "W/mK");
#endif
	}
	catch (IBK::Exception & ex) {
		throw IBK::Exception(ex, "Error creating output files.", FUNC_ID);
	}
	m_profileVector.resize(m_model->m_nElements);

	m_flushTimer.setIntervalLength(2); // only flush every 2 seconds
	m_flushTimer.start();
}


void Outputs::appendOutputs() {

	// write profile outputs
	int outputIdx = 0;

	// index 0 - temperature
	m_profileVector.m_unit.set("K");
	m_profileVector.m_data = m_model->m_T;
	m_profileVector.convert(IBK::Unit("C"));
	m_dataIOs[outputIdx++]->appendData(m_model->m_t, &m_profileVector.m_data[0]);

	// index 1 - relative humidity
	m_profileVector.m_unit.set("---");
	m_profileVector.m_data = m_model->m_rh;
	m_profileVector.convert(IBK::Unit("%"));
	m_dataIOs[outputIdx++]->appendData(m_model->m_t, &m_profileVector.m_data[0]);

	// index 2 - moisture mass density
	m_profileVector.m_unit.set("kg/m3");
	m_profileVector.m_data = m_model->m_rhowv;
	// no unit conversion necessary
	m_dataIOs[outputIdx++]->appendData(m_model->m_t, &m_profileVector.m_data[0]);

	// index 3 - water vapor pressure
	m_profileVector.m_unit.set("Pa");
	m_profileVector.m_data = m_model->m_pv;
	// no unit conversion necessary
	m_dataIOs[outputIdx++]->appendData(m_model->m_t, &m_profileVector.m_data[0]);

#ifdef WRITE_TRANSPORT_FUNCTIONS
	// index 4 - thermal conductivity
	m_profileVector.m_unit.set("W/mK");
	m_profileVector.m_data = m_model->m_lambda;
	// no unit conversion necessary
	m_dataIOs[outputIdx++]->appendData(m_model->m_t, &m_profileVector.m_data[0]);
#endif

	unsigned int n = m_model->m_nElements;
	*m_surfaceValues << m_model->m_t/(24*3600) << '\t'
		 << m_model->m_T[0]-273.15 << '\t' << m_model->m_rh[0]*100 << '\t' << m_model->m_pv[0] << '\t'
		 << m_model->m_q[0] << '\t' << m_model->m_jv[0] << '\t' << m_model->m_jw[0] << '\t'
		 << m_model->m_hv[0] << '\t' << m_model->m_hw[0] << '\t'
		 << m_model->m_T[n-1]-273.15 << '\t' << m_model->m_rh[n-1]*100 << '\t' << m_model->m_pv[n-1] << '\t'
		 << m_model->m_q[n] << '\t' << m_model->m_jv[n] << '\t' << m_model->m_hv[n] << '\n';

	// compute integral moisture mass
	double mwv_int = 0;
	for (unsigned int i=0; i<m_model->m_nElements; ++i) {
		mwv_int += m_model->m_rhowv[i]*m_model->m_dx[i]; // kg/m3 * m = kg/m2
	}
	*m_integralValues << m_model->m_t/(24*3600) << '\t'
		<< mwv_int << std::endl;
	// flush the output stream after at least 1 second or so
	if (m_flushTimer.intervalCompleted()) {
		m_surfaceValues->flush();
		m_integralValues->flush();
	}
}


void Outputs::writeGeofile() {
	const char * const FUNC_ID = "[FUNC_ID]";
	// create new DATAIO::GeoFile instance
	DATAIO::GeoFile geofile;
	// specify file format (true for binary)
	geofile.m_isBinary = false;

	// *** Populate data tables ***

	// compose material data table (the vector materials is from your code)
	for (unsigned int i=0; i<m_model->m_materials.size(); ++i) {
		// GeoFile::Material is a class holding all information of a single material definition
		DATAIO::GeoFile::Material mat;
		mat.name  = m_model->m_materials[i].m_name; // some descriptive name
		//mat.color = ...; // some color, encoded as integer value
		mat.ID    = i; // unique material ID, referenced in elements table below
		// add material definition to list of materials in geofile
		geofile.m_matDefs.push_back(mat);
	}

	// compose element data table (the vector elements is from your code)
	double x = 0;
	for (unsigned int i=0; i<m_model->m_nElements; ++i) {
		// GeoFile::Element holds all element-specific information
		DATAIO::GeoFile::Element el;
		// specify material ID, this ID should be one of the IDs given to materials in
		// the geofile.m_matDefs vector.
		el.matnr = m_model->m_matIdx[i];
		// specify unique element number
		el.n = i;
		// specify grid coordinates (3D grid, coordinate system starting left-top-front)
		el.i = i;
		el.j = 0;
		el.k = 0;
		// specify element center coordinates (typically in [m], 3D grid, coordinate system
		// starting left-bottom-back)
		x += m_model->m_dx[i]*0.5;
		el.x = x;
		el.y = 0.5;
		el.z = 0.5;
		x += m_model->m_dx[i]*0.5;
		// add element information to vector with elements
		geofile.m_elementsVec.push_back(el);
	}

	// compose sides data table (vector sides is from your code)
	x = 0;
	for (unsigned int i=0; i<m_model->m_nElements+1; ++i) {
		// GeoFile::Side holds all side-specific information
		DATAIO::GeoFile::Side sid;
		// specify unique side number
		sid.n = i;
		// specify side orientation
		sid.orientation = 0; // x-direction
		// specify grid line coordinates (see above)
		sid.i = i;
		sid.j = 0;
		sid.k = 0;
		// specify grid line center coordinates (typically in [m], see above)
		sid.x = x;
		sid.y = 0.5;
		sid.z = 0.5;
		if (i< m_model->m_nElements)
			x += m_model->m_dx[i];
		// add side information to vector with sides
		geofile.m_sidesVec.push_back(sid);
	}

	// compose grid data, basically all column, row, stack widths
	geofile.m_grid.xwidths = m_model->m_dx;
	geofile.m_grid.ywidths = std::vector<double>(1,1);
	geofile.m_grid.zwidths = std::vector<double>(1,1);

	// *** Compose file name according to naming convention ***

	IBK::Path projectFilePath(m_model->m_args.m_projectFile);
	std::string projectContent  = IBK::file2String(projectFilePath);
	// suppose the string projectContent holds the relevant data of
	// the project file, at least material references, grid, and assignments

	// compute project file hash code
	unsigned int projectFileHashCode = IBK::SuperFastHash(projectContent);
	std::string projectFileHash = IBK::val2string<unsigned int>(projectFileHashCode);

	// generate relative path to geometry file
	IBK::Path projectFilenameWoExt  = projectFilePath.withoutExtension();
	IBK::Path geofileName = IBK::Path(projectFilenameWoExt.str() + "_" + projectFileHash);
	// append extension based on binary/ASCII format
	if (geofile.m_isBinary)		geofileName.addExtension(".g6b");
	else						geofileName.addExtension(".g6a");
	// and store relative path
	m_geoFileName = geofileName.filename().str();

	// store full path
	geofile.m_filename = m_outputRootPath / geofileName.filename();

	// *** write geometry file ***
	try {
		geofile.write(NULL);
	}
	catch (IBK::Exception & ex) {
		throw IBK::Exception(ex, "Error writing geometry file.", FUNC_ID);
	}

	// compute hash code and remember geofileHash
	// for writing of DataIO containers
	m_geoFileHash = geofile.hashCode();
}


void Outputs::addProfileOutput(const std::string & fname,
							   const std::string & quantity,
							   const std::string & valueUnit)
{
	const char * const FUNC_ID = "[Outputs::addProfileOutput]";

	m_dataIOs.push_back(new DATAIO::DataIO);
	DATAIO::DataIO & d = *m_dataIOs.back();

	// specify filename
	d.m_filename = m_outputRootPath / (fname + ".d6o");
	// specify binary format (for ASCII set false)
	d.m_isBinary = false;
	// specify start year
	d.m_startYear = 2000;
	// set the file data type
	d.m_type = DATAIO::DataIO::T_FIELD; // used for the keyword ELEMENTS or SIDES and in the header
	// store project file name (optional)
	d.m_projectFileName = m_model->m_args.m_projectFile.str();
	// store geometry file and hash of the geometry file content
	d.m_geoFileName = m_geoFileName;
	d.m_geoFileHash = m_geoFileHash;
	// store quantity information
	d.m_quantity = quantity;
	d.m_quantityKeyword = fname;
	// store data format
	d.m_spaceType = DATAIO::DataIO::ST_SINGLE;
	d.m_timeType  = DATAIO::DataIO::TT_NONE;
	// store IO units
	d.m_timeUnit  = "h";
	d.m_valueUnit = valueUnit;
	// store number of values and element numbers (or side numbers, if writing fluxes)
	std::vector<unsigned int> nums(m_model->m_nElements);
	for (unsigned int i=0; i<m_model->m_nElements; ++i)
		nums[i] = i;
	d.m_nums = nums;

	// finally write the file header
	try {
		d.writeHeader();
	}
	catch (IBK::Exception & ex) {
		throw IBK::Exception(ex, "Cannot write output file.", FUNC_ID);
	}
}

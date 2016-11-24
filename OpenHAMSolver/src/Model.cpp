#include "Model.h"

#include <iostream>
#include <algorithm>

#include <IBK_physics.h>
#include <IBK_Exception.h>
#include <IBK_FileReader.h>
#include <IBK_StringUtils.h>
#include <IBK_UnitVector.h>

Model::Model()
{
}

/*! Attempts to extract a value matching the given key, convert it to double and return it. */
template <typename T>
double value(const std::map<std::string, std::string> & keyValuePairs, const std::string & key) {
	const char * const FUNC_ID = "[Model::value]";
	// check if key exists
	std::map<std::string, std::string>::const_iterator it = keyValuePairs.find(key);
	if (it == keyValuePairs.end())
		throw IBK::Exception(IBK::FormatString("Missing option '%1' in project file.").arg(key), FUNC_ID);
	try {
		double val = IBK::string2val<T>(it->second);
		return val;
	}
	catch (...) {
		throw IBK::Exception(IBK::FormatString("Invalid value for option '%1' in project file.").arg(key), FUNC_ID);
	}
}

/*! Attempts to extract a string value matching the given key and return it. */
std::string stringValue(const std::map<std::string, std::string> & keyValuePairs, const std::string & key) {
	const char * const FUNC_ID = "[Model::value]";
	// check if key exists
	std::map<std::string, std::string>::const_iterator it = keyValuePairs.find(key);
	if (it == keyValuePairs.end())
		throw IBK::Exception(IBK::FormatString("Missing option '%1' in project file.").arg(key), FUNC_ID);
	return it->second;
}


void Model::init(const std::string & projectFile) {
	const char * const FUNC_ID = "[Model::init]";

	// read settings from project file
	m_projectFile = projectFile;
	IBK::Path projectFileDir = IBK::Path(projectFile).parentPath();

	std::cout << "Reading project file: " << projectFile << std::endl;
	std::map<std::string, std::string> keyValuePairs;
	readProjectFile(projectFile, keyValuePairs);

	m_maxNonLinIters = value<unsigned int>(keyValuePairs, "maxNonLinIters");

	std::string varMap = stringValue(keyValuePairs, "variableMapping");
	if (varMap == "yz_Tpc")
		m_variableMapping = yz_Tpc;
	else if (varMap == "yz_Trh")
		m_variableMapping = yz_Trh;
	else if (varMap == "yz_TpC")
		m_variableMapping = yz_TpC;

	m_maxDt = value<double>(keyValuePairs, "maxDt");
	m_minDt = value<double>(keyValuePairs, "minDt");

	m_tEnd = value<double>(keyValuePairs, "tEnd");
	m_dt0 = value<double>(keyValuePairs, "dt0");
	m_dtOutput = value<double>(keyValuePairs, "dtOutput");

	// read layer definitions
	unsigned int layerIdx = 0;
	m_nElements = 0;
	m_dx.clear();
	m_matIdx.clear();
	m_materials.clear();
	for (;;) {
		std::string layerName = IBK::FormatString("layer:%1").arg(layerIdx).str();
		// check if layerName is in map
		if (keyValuePairs.find(layerName) == keyValuePairs.end())
			break; // done with layers

		// parse layer definition
		std::string layerDef = stringValue(keyValuePairs, layerName);
		IBK::trim(layerDef, "()");
		std::vector<std::string> tokens;
		IBK::explode(layerDef, tokens, ',', true);
		if (tokens.size() < 2)
			throw IBK::Exception(IBK::FormatString("Bad layer definition of layer '%1' in project file.").arg(layerName), FUNC_ID);
		std::map<std::string, std::string> gridKeyMap;
		for (unsigned int i=0; i<tokens.size(); ++i) {
			std::string kw, val;
			if (!IBK::extract_keyword(tokens[i], kw, val))
				throw IBK::Exception(IBK::FormatString("Bad layer definition of layer '%1' in project file.").arg(layerName), FUNC_ID);
			gridKeyMap[kw] = val;
		}
		std::string mode = stringValue(gridKeyMap, "mode");
		unsigned int materialId = value<unsigned int>(gridKeyMap, "materialId");
		if (mode == "equidistant") {
			unsigned int nElements = value<unsigned int>(gridKeyMap, "nElements");
			double dx = value<double>(gridKeyMap, "dx");

			// add material for this layer
			unsigned int matIdx = m_materials.size();
			m_materials.push_back( Material(materialId) );
			for (unsigned int j=0; j<nElements; ++j) {
				m_dx.push_back(dx);
				m_matIdx.push_back(matIdx);
			}
			m_nElements += nElements;
		}


		++layerIdx;
	}
	if (m_nElements == 0)
		throw IBK::Exception("Missing grid definition in project file.", FUNC_ID);

	m_alphaLeft = value<double>(keyValuePairs, "alphaLeft");
	m_betaLeft = value<double>(keyValuePairs, "betaLeft");

	m_alphaRight = value<double>(keyValuePairs, "alphaRight");
	m_betaRight = value<double>(keyValuePairs, "betaRight");

	try {
		m_TLeft = value<double>(keyValuePairs, "TLeft");
	}
	catch (...) {
		std::string TLeftFile;
		try {
			TLeftFile = stringValue(keyValuePairs, "TLeftFile");
		}
		catch (...) {
			throw IBK::Exception("Expected TLeft or TLeftFile parameter in project file.", FUNC_ID);
		}
		try {
			IBK::Path ccdFile(TLeftFile);
			if (!ccdFile.isAbsolute())
				ccdFile = projectFileDir / ccdFile;
			readCCD(ccdFile, m_TLeftSpline);
		}
		catch (IBK::Exception & ex) {
			throw IBK::Exception(ex, "Error reading data file for parameter TLeftFile.", FUNC_ID);
		}
	}
	m_rhLeft = value<double>(keyValuePairs, "rhLeft");

	m_TRight = value<double>(keyValuePairs, "TRight");
	m_rhRight = value<double>(keyValuePairs, "rhRight");

	std::string RainFluxFile;
	try {
		RainFluxFile = stringValue(keyValuePairs, "RainFluxFile");
	}
	catch (...) {
	}
	if (!RainFluxFile.empty()) {
		try {
			IBK::Path ccdFile(RainFluxFile);
			if (!ccdFile.isAbsolute())
				ccdFile = projectFileDir / ccdFile;
			readCCD(ccdFile, m_gRainSpline);
		}
		catch (IBK::Exception & ex) {
			throw IBK::Exception(ex, "Error reading data file for parameter RainFluxFile.", FUNC_ID);
		}
	}


	// Generic code

	m_t = 0;

	m_u.resize(m_nElements);
	m_rhowv.resize(m_nElements);

	m_zU.resize(m_nElements);
	m_zRhowv.resize(m_nElements);

	m_T.resize(m_nElements);
	m_rh.resize(m_nElements);
	m_pc.resize(m_nElements);
	m_pC.resize(m_nElements);
	m_pv.resize(m_nElements);
	m_pvsat.resize(m_nElements);

	m_Kl.resize(m_nElements);
	m_Kv.resize(m_nElements);
	m_lambda.resize(m_nElements);

	m_q.resize(m_nElements+1);
	m_jv.resize(m_nElements+1);
	m_hv.resize(m_nElements+1);
	m_jw.resize(m_nElements+1);
	m_hw.resize(m_nElements+1);

	// initial variables
	std::vector<double> zInitial(2*m_nElements);
	std::vector<double> yInitial(2*m_nElements);
	for (unsigned int i=0; i<m_nElements; ++i) {
		zInitial[i*2] = 20 + 273.15;
		switch (m_variableMapping) {
			case yz_Trh :
				zInitial[i*2+1] = 0.5; // yz_Trh
				break;
			case yz_Tpc :
				zInitial[i*2+1] = IBK::f_pc_rh(0.5, zInitial[i*2]); // yz_Tpc
				break;
			case yz_TpC :
				zInitial[i*2+1] = IBK::f_log10(-IBK::f_pc_rh(0.5, zInitial[i*2])); // yz_TpC
				break;
			default: ;
		}
	}
	y_z(zInitial,yInitial);
}


void Model::readProjectFile(const std::string & projectFile, std::map<std::string, std::string> & keyValuePairs) {
	const char * const FUNC_ID = "[Model::readProjectFile]";
	std::vector<std::string> lines;
	std::vector<std::string> lastLineTokens; // none, read until end of file
	IBK::FileReader::readAll(IBK::Path(projectFile), lines, lastLineTokens);
	// process all lines
	for (unsigned int i=0; i<lines.size(); ++i) {
		std::string & line = lines[i];
		// remove everything after # character in line
		size_t pos = line.find('#');
		if (pos != std::string::npos)
			line = line.substr(0, pos);
		// skip empty lines
		IBK::trim(line);
		if (line.empty())
			continue;

		// must be a key-value line
		std::string kw, val;
		if (!IBK::extract_keyword(line, kw, val)) {
			throw IBK::Exception( IBK::FormatString("Error in line #%1 of project file '%2', expected <key>=<value> format.")
								  .arg(i+1).arg(projectFile), FUNC_ID);
		}
		if (keyValuePairs.find(kw) != keyValuePairs.end())
			throw IBK::Exception( IBK::FormatString("Error in line #%1 of project file '%2', dublicate keyword '%3'.")
								  .arg(i+1).arg(projectFile).arg(kw), FUNC_ID);
		keyValuePairs[kw] = val;
	}
}


void Model::readCCD(const IBK::Path & ccdFile, IBK::LinearSpline & spline) {
	const char * const FUNC_ID = "[Model::readCCD]";

	// attempt to read the data

#ifdef _MSC_VER
	std::ifstream in(ccdFile.wstr().c_str());
#else
	std::ifstream in(ccdFile.str().c_str());
#endif
	if (!in)
		throw IBK::Exception(IBK::FormatString("Couldn't open climate data file '%1'").arg(ccdFile), FUNC_ID);

	// read all lines up to ID line
	std::string line;
	std::string type, unit;
	while (getline(in, line)) {

		// skip empty lines or lines containing only white spaces
		if (line.find_first_not_of(" \t\r") == std::string::npos)
			continue;
		if (line[0] == '#')
			continue; // skip comments
		std::stringstream strm(line);
		std::string dummy;
		if (strm >> type >> unit) {
			if (strm >> dummy) {
				// we had more string tokens in the line,
				// we must have reached the first data line already
				type.clear();
				unit.clear();
				break;
			}
			else {
				break;
			}
		}
		else {
			throw IBK::Exception(IBK::FormatString("Error reading climate data file '%1'. Invalid file format!").arg(ccdFile), FUNC_ID);
		}
	}
	if (!in) {
		throw IBK::Exception(IBK::FormatString("Error reading climate data file '%1'. Invalid file format!").arg(ccdFile), FUNC_ID);
	}
	// line contains now the first data entry
	IBK::UnitVector		values;
	std::vector<double> timepoints;
	do {
		if (!getline(in, line)) break;
		if (line.find_first_not_of(" \t\r") == std::string::npos || line[0] == '#')
			continue;
		std::replace(line.begin(), line.end(), ':', ' ');
		std::stringstream strm(line);
		int day, hour, minute, sec;
		double value;
		if (strm >> day >> hour >> minute >> sec >> value) {
			double tp = day*24*3600 + 3600*hour + 60*minute + sec;
			timepoints.push_back(tp);
			values.m_data.push_back(value);
		}
		else {
			throw IBK::Exception(IBK::FormatString("Error reading climate data file '%1'. Invalid file format!").arg(ccdFile), FUNC_ID);
		}
	} while (in);

	// try to set the unit
	IBK::Unit io_unit;
	try {  io_unit.set(unit);  }
	catch (...) {
		throw IBK::Exception(IBK::FormatString("Unrecognized unit '%1' in climate file!").arg(unit), FUNC_ID);
	}
	// convert values from io-unit to base unit
	values.m_unit = io_unit;
	if (io_unit.id() != io_unit.base_id())
		values.convert( IBK::Unit(io_unit.base_id()) );
	// set data in linear spline
	spline.setValues(timepoints, values.m_data);


}


void Model::y_z(const std::vector<double> & z, std::vector<double> & y) {
	const char * const FUNC_ID = "[Model::y]";
	// for each balance equation formulate physics

	// z may be a vector of T and pc, or T and rh
	// y is a vector of u and rhowv

	// split balance equation specific solution variables into individual arrays
	for (unsigned int i=0; i<m_nElements; ++i) {
		m_zU[i] = z[i*2];
		m_zRhowv[i] = z[i*2+1];

		try {
			// compute model state based on variable mapping
			switch (m_variableMapping) {
				case yz_Tpc :
				case yz_Trh :
				case yz_TpC :
					updateStatesTx(i);
					break;
				default:
					throw IBK::Exception("Not implemented", FUNC_ID);
			}
		}
		catch (IBK::Exception & ex) {
			throw IBK::Exception(ex, IBK::FormatString("Error computing values in element %1").arg(i), FUNC_ID);
		}
		// store computed conserved quantities back in common array
		y[i*2] = m_u[i];
		y[i*2+1] = m_rhowv[i];
	}
}


void Model::ydot_z(double t, const std::vector<double> & z, std::vector<double> &y, std::vector<double> & ydot) {
	// update y
	y_z(z,y);

	m_t = t;

	// boundary conditions
	updateBoundaryConditions();

	// update fluxes
	updateFluxes();

	// update divergences
	for (unsigned int i=0; i<m_nElements; ++i) {
		double qSumLeft = m_q[i] + m_hv[i] + m_hw[i];
		double qSumRight = m_q[i+1] + m_hv[i+1] + m_hw[i+1];
		double divU = (qSumLeft - qSumRight)/m_dx[i];
		ydot[i*2] = divU;

		double jSumLeft = m_jv[i] + m_jw[i];
		double jSumRight = m_jv[i+1] + m_jw[i+1];
		double divRhowv = (jSumLeft - jSumRight)/m_dx[i];
		ydot[i*2+1] = divRhowv;
	}


}


void Model::updateStatesTx(unsigned int e) {
	const Material & mat = m_materials[ m_matIdx[e] ];

	double T = m_zU[e];
	double pc, pC, rh, Ol;
	double moistureMassDensity;

	switch (m_variableMapping) {
		case yz_Tpc :
			pc = m_zRhowv[e];
			rh = IBK::f_relhum(T, pc);
			pC = IBK::f_log10(-pc);
			break;

		case yz_Trh :
			rh = m_zRhowv[e];
			pc = IBK::f_pc_rh(rh, T); // this value is clipped for RH < MIN_RH and RH > 1
			pC = IBK::f_log10(-pc);
			break;

		case yz_TpC :
			pC = m_zRhowv[e];
			pc = -IBK::f_pow10(pC);
			rh = IBK::f_relhum(T, pc);
			break;
		default:;
	}


	// saturation pressure in [Pa]
	double pvsat = IBK::f_psat(T);
	// partial vapor pressure  in [Pa]
	double pv = pvsat * rh;

	if ( mat.m_isAir ) {

		// moisture mass density = vapor mass density in [kg/m3]
		moistureMassDensity = pv / ( IBK::R_VAPOR * T);

		/// \todo
	}
	// for regular materials we need to use either the sorption isotherm, or the
	// moisture retention function
	else {

		// first compute capillary pressures in [Pa] for relative humidity
		Ol = mat.Ol_pc( pc );
		moistureMassDensity = IBK::RHO_W*Ol;

	} // if AIR-Material

	// specific heat capacity in [J/kgK]
	double ce = mat.m_cT;
	// bulk material density in [kg/m3]
	double bulk_density = mat.m_rho;

	m_T[e] = T;
	m_rh[e] = rh;
	m_pc[e] = pc;
	m_pC[e] = pC;
	m_pv[e] = pv;
	m_pvsat[e] = pvsat;

	m_lambda[e] = mat.lambda_Ol(Ol);
	m_Kl[e] = mat.Kl_Ol(Ol);
	m_Kv[e] = mat.Kv_Ol(T, Ol);

	// compute energy density for moist material in [J/m3]
	m_u[e] = T*( ce*bulk_density + IBK::C_WATER*moistureMassDensity );
	m_rhowv[e] = moistureMassDensity;
}


void Model::updateBoundaryConditions() {
	// boundary fluxes are always defined positively in coordinate direction
	// surface value extrapolation is not used

	// update time-dependent climate data
	if (!m_TLeftSpline.empty())
		m_TLeft = m_TLeftSpline.value(m_t);
	double gRain = 0;
	if (!m_gRainSpline.empty())
		gRain = m_gRainSpline.value(m_t);


	// Left is ambient climate
	m_qLeft = m_alphaLeft*(m_TLeft - m_T[0]);
	double pvLeft = IBK::f_pv(m_TLeft, m_rhLeft);
	m_jvLeft = m_betaLeft*(pvLeft - m_pv[0]);
	// for enthalpy use upwinding
	if (m_jvLeft > 0)
		m_hvLeft = m_jvLeft*(IBK::C_VAPOR*m_TLeft + IBK::H_EVAP); // outside in
	else
		m_hvLeft = m_jvLeft*(IBK::C_VAPOR*m_T[0] + IBK::H_EVAP); // inside out

	// for rain
	if (gRain != 0) {
		m_jwLeft = gRain;
		// for enthalpy use upwinding
		if (m_jwLeft > 0)
			m_hwLeft = m_jwLeft*IBK::C_WATER*m_TLeft; // outside in
		else
			m_hwLeft = m_jwLeft*IBK::C_WATER*m_T[0]; // inside out
	}
	else {
		m_jwLeft = 0;
		m_hwLeft = 0;
	}


	unsigned int n1 = m_nElements-1;
	// Right is indoor climate
	m_qRight = m_alphaRight*(m_T[n1] - m_TRight);
	double pvRight = IBK::f_pv(m_TRight, m_rhRight);
	m_jvRight = m_betaRight*(m_pv[n1] - pvRight);
	// for enthalpy use upwinding
	if (m_jvRight > 0)
		m_hvRight = m_jvRight*(IBK::C_VAPOR*m_T[n1] + IBK::H_EVAP); // inside out
	else
		m_hvRight = m_jvRight*(IBK::C_VAPOR*m_TRight + IBK::H_EVAP); // outside in

	// store in flux vectors
	m_q[0] = m_qLeft;
	m_jv[0] = m_jvLeft;
	m_hv[0] = m_hvLeft;
	m_jw[0] = m_jwLeft;
	m_hw[0] = m_hwLeft;

	m_q[m_nElements] = m_qRight;
	m_jv[m_nElements] = m_jvRight;
	m_hv[m_nElements] = m_hvRight;
	m_jw[m_nElements] = 0;
	m_hw[m_nElements] = 0;

}


void Model::updateFluxes() {
	for (unsigned int i=1; i<m_nElements; ++i) {
		unsigned int eL = i-1;
		unsigned int eR = i;
		double gx2 = m_dx[eL] + m_dx[eR]; // twice the distance between element centers


		// heat conduction flux

		double RL = m_dx[eL]/m_lambda[eL];
		double RR = m_dx[eR]/m_lambda[eR];

		double R = (RL + RR)/2;
		double q = 1.0/R * (m_T[eL] - m_T[eR]);
		m_q[i] = q;


		// vapor diffusion
		double KV_mean = (m_dx[eL]*m_Kv[eL] + m_dx[eR]*m_Kv[eR])/gx2;
		m_jv[i] = KV_mean*(m_pv[eL] - m_pv[eR])/(0.5*gx2);
		if (m_jv[i] > 0)
			m_hv[i] = m_jv[i]*(IBK::C_VAPOR*m_T[eL] + IBK::H_EVAP);
		else
			m_hv[i] = m_jv[i]*(IBK::C_VAPOR*m_T[eR] + IBK::H_EVAP);


		// liquid conduction
		double Kl_mean = (m_dx[eL]*m_Kl[eL] + m_dx[eR]*m_Kl[eR])/gx2;
		m_jw[i] = Kl_mean*(m_pc[eL] - m_pc[eR])/(0.5*gx2);
		if (m_jw[i] > 0)
			m_hw[i] = m_jw[i]*IBK::C_WATER*m_T[eL];
		else
			m_hw[i] = m_jw[i]*IBK::C_WATER*m_T[eR];
	}
}



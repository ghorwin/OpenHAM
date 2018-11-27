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

#include "Model.h"

#include <iostream>
#include <algorithm>

#include <IBK_messages.h>
#include <IBK_physics.h>
#include <IBK_Exception.h>
#include <IBK_FileReader.h>
#include <IBK_StringUtils.h>
#include <IBK_UnitVector.h>

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


void Model::init(const IBK::SolverArgsParser & args) {
	const char * const FUNC_ID = "[Model::init]";

	// set default settings
	m_maxNonLinIters = 4;

	m_variableMapping = yz_TpC;

	try {
		// read settings from project file
		m_args = args;

		IBK::IBK_Message( IBK::FormatString("Initializing project\n"), IBK::MSG_PROGRESS, FUNC_ID, IBK::VL_STANDARD);
		IBK::MessageIndentor indent; (void)indent;
		m_project.readXML(args.m_projectFile);

		m_maxDt		= m_project.m_maxTimeStep.value; // in seconds
		m_minDt		= m_project.m_minTimeStep.value; // in seconds

		m_t			= m_project.m_simStart.value; // in seconds
		m_tEnd		= m_t + m_project.m_simDuration.value; // in seconds
		m_dt0		= m_project.m_initialTimeStep.value; // in seconds
		m_dtOutput	= m_project.m_dtOutput.value; // in seconds

		m_nElements = 0;
		m_dx.clear();
		m_matIdx.clear();
		m_materials.clear();

		double gridSpacing = 0.001; // 1 mm grid spacing for now

		{
			IBK::IBK_Message( IBK::FormatString("Setting up layers\n"), IBK::MSG_PROGRESS, FUNC_ID, IBK::VL_STANDARD);
			IBK::MessageIndentor indent2; (void)indent2;
			// process material layers
			for (unsigned int i=0; i<m_project.m_materialLayers.size(); ++i) {
				IBK::IBK_Message( IBK::FormatString("Layer #%1:\n").arg(i+1), IBK::MSG_PROGRESS, FUNC_ID, IBK::VL_STANDARD);
				IBK::MessageIndentor indent3; (void)indent3;
				const DELPHIN_LIGHT::Project::MaterialLayer & matLayer = m_project.m_materialLayers[i];
				// lookup and read material data
				if (matLayer.m_matRef == NULL)
					throw IBK::Exception(IBK::FormatString("Missing material assignment to material layer #%1.").arg(i), FUNC_ID);
				std::string matref = matLayer.m_matRef->m_filename.str();
				int matIdx = 0;
				if (matref.find("built-in:") == 0) {
					matIdx = IBK::string2val<int>(matref.substr(9));
					IBK::IBK_Message(IBK::FormatString("Built-in material #%1\n").arg(matIdx), IBK::MSG_PROGRESS, FUNC_ID, IBK::VL_STANDARD);
					Material m;
					m.init(matIdx);
					m_materials.push_back(m);
				}
				else {
					IBK::IBK_Message(IBK::FormatString("Material file reference: %1\n").arg(matref), IBK::MSG_PROGRESS, FUNC_ID, IBK::VL_STANDARD);
					// resolve path placeholder and read material file
					IBK::Path fullMatFilePath = IBK::Path(matref).withReplacedPlaceholders(m_project.m_placeholders);
					Material m;
					try {
						m.readFromFile(fullMatFilePath);
					}
					catch (IBK::Exception & ex) {
						throw IBK::Exception(ex, IBK::FormatString("Error initializing material from file."), FUNC_ID);
					}
					m_materials.push_back(m);
				}
				// apply discretization

				// currently only equidistant grid spacing supported, should be sufficient for most cases
				unsigned int n = static_cast<unsigned int>(matLayer.m_width.value / gridSpacing);
				// special case, width < gridSpacing*3
				if (n < 3)
					n = 3;
				std::size_t materialIndex = m_materials.size()-1;
				double dx = matLayer.m_width.value / n;
				for (unsigned int j=0; j<n; ++j) {
					m_dx.push_back(dx);
					m_matIdx.push_back(materialIndex);
				}
				m_nElements += n;
				IBK::IBK_Message( IBK::FormatString("d  = %1 m\n").arg(matLayer.m_width.value), IBK::MSG_PROGRESS, FUNC_ID, IBK::VL_STANDARD);
				IBK::IBK_Message( IBK::FormatString("n  = %1\n").arg(n), IBK::MSG_PROGRESS, FUNC_ID, IBK::VL_STANDARD);
				IBK::IBK_Message( IBK::FormatString("dx = %1 m\n").arg(dx), IBK::MSG_PROGRESS, FUNC_ID, IBK::VL_STANDARD);

				// dump out material functions for plotting
				m_materials.back().createPlots(m_dirs.m_varDir, i+1);
			}
		}

		// transfer constant climate conditions
		m_TLeft = m_project.m_leftInterface.T.value;
		m_rhLeft = m_project.m_leftInterface.RH.value;
		m_TRight = m_project.m_rightInterface.T.value;
		m_rhRight = m_project.m_rightInterface.RH.value;

		if (m_nElements == 0)
			throw IBK::Exception("Missing grid definition in project file.", FUNC_ID);


		// Generic code

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
		IBK::IBK_Message( IBK::FormatString("Initial conditions\n"), IBK::MSG_PROGRESS, FUNC_ID, IBK::VL_STANDARD);
		IBK::MessageIndentor indent2; (void)indent2;
		IBK::IBK_Message( IBK::FormatString("T  = %1 C\n").arg(m_project.m_initialT.value - 273.15), IBK::MSG_PROGRESS, FUNC_ID, IBK::VL_STANDARD);
		IBK::IBK_Message( IBK::FormatString("RH = %1 %\n").arg(m_project.m_initialRH.value*100), IBK::MSG_PROGRESS, FUNC_ID, IBK::VL_STANDARD);

		std::vector<double> zInitial(2*m_nElements);
		std::vector<double> yInitial(2*m_nElements);
		for (unsigned int i=0; i<m_nElements; ++i) {
			zInitial[i*2] = m_project.m_initialT.value;
			switch (m_variableMapping) {
				case yz_Trh :
					zInitial[i*2+1] = m_project.m_initialRH.value; // yz_Trh
					break;
				case yz_Tpc :
					zInitial[i*2+1] = IBK::f_pc_rh(m_project.m_initialRH.value, zInitial[i*2]); // yz_Tpc
					break;
				case yz_TpC :
					zInitial[i*2+1] = IBK::f_log10(-IBK::f_pc_rh(m_project.m_initialRH.value, zInitial[i*2])); // yz_TpC
					break;
				default: ;
			}
		}
		// evaluated mapping function y(z)
		y_z(zInitial,yInitial);
	}
	catch (IBK::Exception & ex) {
		throw IBK::Exception(ex, "Error initializing project.", FUNC_ID);
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
	const char * const FUNC_ID = "[Model::y_z]";
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
	const char * const FUNC_ID = "[Model::updateStatesTx]";

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

		default:
			throw IBK::Exception("Uninitialized m_variableMapping", FUNC_ID);
	}


	// saturation pressure in [Pa]
	double pvsat = IBK::f_psat(T);
	// partial vapor pressure  in [Pa]
	double pv = pvsat * rh;

	if ( mat.m_isAir ) {

		// moisture mass density = vapor mass density in [kg/m3]
		moistureMassDensity = pv / ( IBK::R_VAPOR * T);
		Ol = moistureMassDensity / IBK::RHO_W;
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
	if (!m_project.m_leftInterface.T_spline.empty())
		m_TLeft = m_project.m_leftInterface.T_spline.value(m_t);
//	double gRain = 0;
//	if (!m_gRainSpline.empty())
//		gRain = m_gRainSpline.value(m_t);


	// Left is ambient climate
	m_qLeft = m_project.m_leftInterface.alpha.value*(m_TLeft - m_T[0]);
	double pvLeft = IBK::f_pv(m_TLeft, m_rhLeft);
	m_jvLeft = m_project.m_leftInterface.beta.value*(pvLeft - m_pv[0]);
	// for enthalpy use upwinding
	if (m_jvLeft > 0)
		m_hvLeft = m_jvLeft*(IBK::C_VAPOR*m_TLeft + IBK::H_EVAP); // outside in
	else
		m_hvLeft = m_jvLeft*(IBK::C_VAPOR*m_T[0] + IBK::H_EVAP); // inside out

	// for rain
//	if (gRain != 0) {
//		m_jwLeft = gRain;
//		// for enthalpy use upwinding
//		if (m_jwLeft > 0)
//			m_hwLeft = m_jwLeft*IBK::C_WATER*m_TLeft; // outside in
//		else
//			m_hwLeft = m_jwLeft*IBK::C_WATER*m_T[0]; // inside out
//	}
//	else {
		m_jwLeft = 0;
		m_hwLeft = 0;
//	}


	unsigned int n1 = m_nElements-1;
	// Right is indoor climate
	m_qRight = m_project.m_rightInterface.alpha.value*(m_T[n1] - m_TRight);
	double pvRight = IBK::f_pv(m_TRight, m_rhRight);
	m_jvRight = m_project.m_rightInterface.beta.value*(m_pv[n1] - pvRight);
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



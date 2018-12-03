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

#include "Material.h"

#include <IBK_math.h>
#include <IBK_FileReader.h>
#include <IBK_StringUtils.h>
#include <IBK_Parameter.h>
#include <IBK_messages.h>
#include <IBK_physics.h>

Material::Material() :
	m_i(-1)
{
}


void Material::init(int index) {
//	const char * const FUNC_ID = "[Material::init(int index)]";
	m_i = index;
	std::string logKlSplineOl;
	std::string logKlSplineLog10Kl;
	switch (index) {
		// Hamstad Brick
		case 0 :
			m_name		= "Hamstad Brick";
			m_rho		= 2005;
			m_cT		= 840;
			m_lambda	= 0.5;
			m_Opor		= 0.157;
			m_Oeff		= 0.157;
			m_isAir		= false;
			logKlSplineOl =
				"0                8.03206e-05      9.61229e-05      0.000115034      0.000137666      0.000164751      0.000197164      0.000235954      0.000282375      0.00033793       0.000404414      0.000483978      0.000579195      0.000693143      0.000829509      0.000992699      0.00118799       0.00142168       0.00170133       0.00203595       0.00243632       0.00291528       0.00348815       0.00417313       0.00499174       0.00596927       0.00713511       0.00852281       0.0101696        0.0121147        0.014396         0.017045         0.0200829        0.0235369        0.027555         0.0329148        0.042885         0.0670305        0.109759         0.140851         0.151343         0.154359         0.155487         0.156406         0.156622         0.157            ";
			logKlSplineLog10Kl =
				"-20.5031         -20.5031         -20.1831         -19.8631         -19.5431         -19.2231         -18.9031         -18.5831         -18.2631         -17.9431         -17.6231         -17.3031         -16.9831         -16.6631         -16.3431         -16.0231         -15.8054         -15.5378         -15.3401         -15.158          -14.636          -14.4122         -14.1042         -13.834          -13.4157         -13.2141         -13.0024         -12.7079         -12.5578         -12.4636         -12.3941         -12.2614         -12.0959         -12.0408         -11.8986         -11.8845         -11.773          -11.1372         -10.1465         -9.37365         -8.92297         -8.82647         -8.72144         -8.72144         -8.71975         -8.71975";

			break;

		// Hamstad Finishing Material
		case 1 :
			m_name		= "Hamstad Finishing Material";
			m_rho		= 790;
			m_cT		= 870;
			m_lambda	= 0.2;
			m_Opor		= 0.209;
			m_Oeff		= 0.209;
			m_isAir		= false;
			break;

		// EN 15026 Material
		case 2 :
			m_name		= "EN 15026 Material";
			m_rho		= 1824;
			m_cT		= 1000;
			m_lambda	= 1.5;
			m_Oeff		= 0.146;
			m_Opor		= 0.146;
			m_isAir		= false;
			break;

		// invalid number
		default:
			m_i = -1;
			m_isAir		= false;
	}
	if (!logKlSplineOl.empty()) {
		m_lgKl_Ol_Spline.read(logKlSplineOl, logKlSplineLog10Kl);
	}
}


void extractKeyValuePairs(const std::vector<std::string> & lines, std::map<std::string, std::string> & keyValuePairs) {
	const char * const FUNC_ID = "[extractKeyValuePairs]";
	keyValuePairs.clear();
	// process all lines
	for (unsigned int i=0; i<lines.size(); ++i) {
		std::string line = lines[i];
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
			continue;
		}
		if (keyValuePairs.find(kw) != keyValuePairs.end())
			throw IBK::Exception( IBK::FormatString("Duplicate keyword '%1'.")
								  .arg(kw), FUNC_ID);
		keyValuePairs[kw] = val;
	}
}


void Material::readFromFile(const IBK::Path & m6FilePath) {
	const char * const FUNC_ID = "[Material::readFromFile]";

	m_filepath = m6FilePath;

	m_i = -1;
	m_isAir = false;

	m_mew.clear();
	m_Oeff = 0;
	m_Opor = 0;

	try {
		std::vector<std::string> lines;
		std::vector<std::string> lastLineTokens; // none, read until end of file
		IBK::FileReader::readAll(m6FilePath, lines, lastLineTokens);

		// check file version
		if (lines.empty() || lines[0].find("D6MARLZ! 006.00") != 0)
			throw IBK::Exception("Invalid material file format.", FUNC_ID);

		std::vector<std::string> lines2(lines.begin()+1, lines.end());

		// split known sections
		std::vector<std::string> section_titles;
		std::vector<std::vector<std::string> > section_data;

		section_titles.push_back("IDENTIFICATION");
		section_titles.push_back("STORAGE_BASE_PARAMETERS");
		section_titles.push_back("TRANSPORT_BASE_PARAMETERS");
		section_titles.push_back("MOISTURE_STORAGE");
		section_titles.push_back("MOISTURE_TRANSPORT");
		section_titles.push_back("HEAT_TRANSPORT");
		IBK::explode_sections(lines2, section_titles, section_data, "#");

		// process identification section
		std::map<std::string, std::string> keyValuePairs;
		extractKeyValuePairs(section_data[0], keyValuePairs);
		m_name = keyValuePairs["NAME"];
		IBK::IBK_Message(IBK::FormatString("NAME = %1\n").arg(m_name), IBK::MSG_PROGRESS, FUNC_ID, IBK::VL_INFO);
		m_flags = keyValuePairs["FLAGS"];
		IBK::IBK_Message(IBK::FormatString("FLAGS = %1\n").arg(m_flags), IBK::MSG_PROGRESS, FUNC_ID, IBK::VL_INFO);

		// storage base parameters
		try {
			extractKeyValuePairs(section_data[1], keyValuePairs);
			IBK::Parameter p;
			p.set("RHO", keyValuePairs["RHO"]); m_rho = p.value;
			p.set("CE", keyValuePairs["CE"]); m_cT = p.value;
			p.set("THETA_POR", keyValuePairs["THETA_POR"]); m_Opor = p.value;
			p.set("THETA_EFF", keyValuePairs["THETA_EFF"]); m_Oeff = p.value;

			if (m_Oeff <= 0)
				throw IBK::Exception("Invalid or missing THETA_EFF parameter.", FUNC_ID);
		}
		catch (IBK::Exception & ex) {
			throw IBK::Exception(ex, "Error reading storage base parameters.", FUNC_ID);
		}

		// transport base parameters
		try {
			extractKeyValuePairs(section_data[2], keyValuePairs);
			IBK::Parameter p;
			p.set("LAMBDA", keyValuePairs["LAMBDA"]); m_lambda = p.value;
			if (!keyValuePairs["MEW"].empty())
				m_mew.set("MEW", keyValuePairs["MEW"]);
		}
		catch (IBK::Exception & ex) {
			throw IBK::Exception(ex, "Error reading transport base parameters.", FUNC_ID);
		}

		// moisture storage function
		try {
			for (unsigned int i=0; i<section_data[3].size(); ++i) {
				std::string line = section_data[3][i];
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
				if (IBK::extract_keyword(line, kw, val)) {
					if (kw == "FUNCTION") {
						if (val == "Theta_l(pC)_de") {
							std::string xvals = section_data[3][i+1];
							std::string yvals = section_data[3][i+2];
							// set linear spline
							try {
								m_Ol_pC_Spline.read(xvals, yvals);
							}
							catch (IBK::Exception & ex) {
								throw IBK::Exception(ex, IBK::FormatString("Error parsing 'Theta_l(pC)_de' spline."), FUNC_ID);
							}
							std::string errmsg;
							if (!m_Ol_pC_Spline.valid()) {
								m_Ol_pC_Spline.makeSpline(errmsg);
								throw IBK::Exception(IBK::FormatString("Invalid spline data: %1").arg(errmsg), FUNC_ID);
							}
						}
						i += 2; // skip the next two lines
					} // kw == "FUNCTION"
				}
			}
		}
		catch (IBK::Exception & ex) {
			throw IBK::Exception(ex, "Error reading moisture storage.", FUNC_ID);
		}


		// moisture transport functions
		try {
			for (unsigned int i=0; i<section_data[4].size(); ++i) {
				std::string line = section_data[4][i];
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
				if (IBK::extract_keyword(line, kw, val)) {
					if (kw == "FUNCTION") {
						std::string xvals = section_data[4][i+1];
						std::string yvals = section_data[4][i+2];
						// set linear spline
						IBK::LinearSpline spl;
						try {
							spl.read(xvals, yvals);
						}
						catch (IBK::Exception & ex) {
							throw IBK::Exception(ex, IBK::FormatString("Error parsing '%1' spline.").arg(val), FUNC_ID);
						}
						std::string errmsg;
						if (!spl.valid()) {
							spl.makeSpline(errmsg);
							throw IBK::Exception(IBK::FormatString("Invalid spline data: %1").arg(errmsg), FUNC_ID);
						}

						if (val == "lgKl(Theta_l)") {
							m_lgKl_Ol_Spline.swap(spl);
						}
						else if (val == "lgKv(Theta_l)") {
							m_lgKv_Ol_Spline.swap(spl);
						}
						else
							throw IBK::Exception(IBK::FormatString("Unknown or unsupported moisture transport function '%1'.").arg(val), FUNC_ID);
						i += 2; // skip the next two lines
					} // kw == "FUNCTION"
				}
			}
		}
		catch (IBK::Exception & ex) {
			throw IBK::Exception(ex, "Error reading moisture transport section.", FUNC_ID);
		}

		// thermal transport functions
		try {
			for (unsigned int i=0; i<section_data[5].size(); ++i) {
				std::string line = section_data[5][i];
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
				if (IBK::extract_keyword(line, kw, val)) {
					if (kw == "FUNCTION") {
						std::string xvals = section_data[5][i+1];
						std::string yvals = section_data[5][i+2];
						// set linear spline
						IBK::LinearSpline spl;
						try {
							spl.read(xvals, yvals);
						}
						catch (IBK::Exception & ex) {
							throw IBK::Exception(ex, IBK::FormatString("Error parsing '%1' spline.").arg(val), FUNC_ID);
						}
						std::string errmsg;
						if (!spl.valid()) {
							spl.makeSpline(errmsg);
							throw IBK::Exception(IBK::FormatString("Invalid spline data: %1").arg(errmsg), FUNC_ID);
						}

						if (val == "lambda(Theta_l)") {
							m_lambda_Ol_Spline.swap(spl);
						}
						else
							throw IBK::Exception(IBK::FormatString("Unknown or unsupported thermal transport function '%1'.").arg(val), FUNC_ID);
						i += 2; // skip the next two lines
					} // kw == "FUNCTION"
				}
			}
		}
		catch (IBK::Exception & ex) {
			throw IBK::Exception(ex, "Error reading thermal transport section.", FUNC_ID);
		}

	}
	catch (IBK::Exception & ex) {
		throw IBK::Exception(ex, IBK::FormatString("Error reading material file '%1'.").arg(m6FilePath), FUNC_ID);
	}
}


double Material::Ol_pc(double pc) const {
	double Ol = 0;
	switch (m_i) {
		// Hamstad Brick
		case 0 :
		{
			const double reta[2] = {-1.25e-5, -1.8e-5};
			const double retn[2] = {1.65, 6};
			const double retm[2] = {0.39394, 0.83333};
			const double retw[2] = {0.3, 0.7};
			for (int i=0; i<2; ++i) {
				double tmp = IBK::f_pow(reta[i]*pc, retn[i]);
				double tmp2 = IBK::f_pow(1.0 + tmp, retm[i]);
				Ol += retw[i] / tmp2;
			}
			Ol *= m_Oeff;
			return Ol;
		}

		// Hamstad Finishing Material
		case 1 :
		{
			double tmp = IBK::f_pow(-2e-6*pc, 1.27);
			double tmp2 = IBK::f_pow(1.0 + tmp, 0.21260);
			Ol = m_Oeff / tmp2;
			return Ol;
		}

		// EN 15026 Material
		case 2 :
		{
			return 0.146/IBK::f_pow(1+IBK::f_pow(-8e-8*pc, 1.6),0.375);
		}

	}
	// must be a material from data file, use linear spline interpolation
	return m_Ol_pC_Spline.value(IBK::f_log10(-pc));
}


double Material::lambda_Ol(double Ol) const {
	switch (m_i) {
		// Hamstad Brick
		case 0 : return m_lambda + 4.5 * Ol;
		// Hamstad Finishing Material
		case 1 : return m_lambda + 4.5 * Ol;
		// EN 15026 Material
		case 2 : return m_lambda + 15.8 * Ol;
	}
	// use standard model
	if (m_Ol_pC_Spline.valid())
		return m_Ol_pC_Spline.value(Ol);
	else
		return m_lambda + 0.56*Ol;
}


double Material::Kl_Ol(double Ol) const {
	switch (m_i) {
		// Hamstad Brick
		case 0 :
			return IBK::f_pow10(m_lgKl_Ol_Spline.value(Ol));

		// Hamstad Finishing Material
		case 1 :
		{
			double tmp = (Ol-0.120)*1000;
			tmp = -33.0 + 7.04e-2*tmp - 1.742e-4*tmp*tmp - 2.7953e-6*tmp*tmp*tmp
				  -1.1566e-7*tmp*tmp*tmp*tmp + 2.5969e-9*tmp*tmp*tmp*tmp*tmp;
			tmp *= 0.4342944819032518;
			return IBK::f_pow10(tmp);
		}

		// EN 15026 Material
		case 2 :
		{
			double w2 = Ol*1000-73;
			double lnK = -39.2619 + w2*(0.0704 + w2*(-1.7420e-4 + w2*(-2.7953e-6 + w2*(-1.1566e-7 + w2*2.5969e-9))));
			return IBK::f_exp(lnK);
		}

	}
	return IBK::f_pow10(m_lgKl_Ol_Spline.value(Ol));
}


double Material::Kv_Ol(double T, double Ol) const {
	const char * const FUNC_ID = "[Material::Kv_Ol]";

	switch (m_i) {
		// Hamstad Brick
		case 0 :
		{
			const double mew = 30.0;
			double tmp = 1.0 - Ol/m_Oeff;
			double Kv = 2.61e-5/(461.89*T*mew) * tmp/(0.503*tmp*tmp + 0.497);
			return Kv;
		}

		// Hamstad Finishing Material
		case 1 :
		{
			const double mew = 3.0;
			double tmp = 1.0 - Ol/m_Oeff;
			double Kv = 2.61e-5/(461.89*T*mew) * tmp/(0.503*tmp*tmp + 0.497);
			return Kv;
		}

		// EN 15026 Material
		case 2 :
		{
			double normalizedW = 1 - Ol/0.146;
			const double mew = 200;
			return IBK::DV_AIR/(IBK::R_VAPOR*293.15*mew)*normalizedW/(0.503*normalizedW*normalizedW + 0.497);
		}
	}
	if (m_lgKv_Ol_Spline.valid())
		return IBK::f_pow10(m_lgKv_Ol_Spline.value(Ol));
	if (m_mew.name.empty())
		throw IBK::Exception("Missing parametrization for Kv(Ol), neither lgKv(Ol) spline nor MEW parameter provided.", FUNC_ID);
	else
		return IBK::DV_AIR/(IBK::R_VAPOR*293.15*m_mew.value)*(1-Ol/m_Oeff);
}

// open file

void openFileUtf8(const IBK::Path & fpath, std::ofstream & fout) {
	#if defined(_WIN32)
	#if defined(_MSC_VER)
		fout.open( fpath.wstr().c_str(), std::ios_base::binary);
	#else
		std::string filenameAnsi = IBK::WstringToANSI(fpath.wstr(), false);
		fout.open( filenameAnsi.c_str(), std::ios_base::binary);
	#endif
	#else
		fout.open( fpath.c_str() );
	#endif
}

const char * const MRC_GNUPLOT =
	"set terminal postscript monochrome\n"
	"set output  \"layer_${IDX}_mrc.ps\"\n"
	"set title \"Moisture retention function - Layer ${IDX}\"\n"
	"set xlabel \"log10(-pc)\"\n"
	"set ylabel \"Moisture content [kg/m3]\"\n"
	"set key off\n"
	"plot \"layer_${IDX}_mrc.dat\" using 1:2 with lines\n"
	"\n"
	"set output  \"layer_${IDX}_sorp.ps\"\n"
	"set title \"Sorption isotherm - Layer ${IDX}\"\n"
	"set xlabel \"Relative humidity [%]\"\n"
	"set key off\n"
	"plot \"layer_${IDX}_mrc.dat\" using 3:2 with lines\n"
	"\n";

void Material::createPlots(const IBK::Path & plotDir, unsigned int layerIdx) const {
	// generate material file name
	IBK::Path fileNameRoot = plotDir / IBK::FormatString("layer_%1").arg(layerIdx).str();

	IBK::Path mrcDataFile(fileNameRoot + "_mrc.dat");
	std::ofstream fout;
	openFileUtf8(mrcDataFile, fout);

	unsigned int NUM_VALUES = 100;
	for (unsigned int i=0; i<NUM_VALUES; ++i) {
		double pC = 10.0*i/NUM_VALUES;
		double pc = -IBK::f_pow10(pC);
		double rh = IBK::f_relhum(293.15, pc);
		fout << pC << " " << Ol_pc(-IBK::f_pow10(pC)) << " " << rh*100 << std::endl;
	}
	fout.close();

	// create corresponding gnuplot file
	mrcDataFile = IBK::Path(fileNameRoot + "_mrc.gpi");
	openFileUtf8(mrcDataFile, fout);

	std::string mrc_plot_text = MRC_GNUPLOT;
	mrc_plot_text = IBK::replace_string(mrc_plot_text, "${IDX}", IBK::val2string(layerIdx));
	fout << mrc_plot_text;
	fout.close();


	IBK::Path dataFile(fileNameRoot + "_transport.dat");
	openFileUtf8(dataFile, fout);

}

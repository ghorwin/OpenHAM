/*	Copyright (c) 2001-2017, Institut für Bauklimatik, TU Dresden, Germany

	Written by A. Nicolai, H. Fechner, St. Vogelsang, A. Paepcke, J. Grunewald
	All rights reserved.

	This file is part of the CCM Library.

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

#include "CCM_ClimateDataLoader.h"

#include <fstream>
#include <algorithm>

#include <IBK_assert.h>
#include <IBK_Constants.h>
#include <IBK_Exception.h>
#include <IBK_FileReader.h>
#include <IBK_FormatString.h>
#include <IBK_InputOutput.h>
#include <IBK_messages.h>
#include <IBK_MultiLanguageString.h>
#include <IBK_Parameter.h>
#include <IBK_physics.h>
#include <IBK_StringUtils.h>
#include <IBK_Time.h>
#include <IBK_Unit.h>
#include <IBK_UnitVector.h>
#include <IBK_CSVReader.h>

#include <fstream>
#include <iostream>

#include <tinyxml.h>

#include "CCM_Constants.h"
#include "CCM_SolarRadiationModel.h"

#include "CCM_Defines.h"

namespace CCM {

/*! mask the major version number. */
inline unsigned int majorMaskBinary(){ return 0x000000ff; }

/*! mask the minor version number. */
inline unsigned int minorMaskBinary(){ return 0x0000ff00; }

/*! Returns the complete version number as unsigned int formatted for a binary output little endian. */
inline unsigned int versionNumberBinary(){ return ( MAJOR_VERSION_NUMBER | (MINOR_VERSION_NUMBER << 8 ) ); }

/*! Extracts major number from version id
	\param version The version number read from an output file.
*/
inline unsigned int majorFromVersionBinary( unsigned int version ){
		return ( version & majorMaskBinary() );
}

/*! Extracts major number from version id
	\param version The version number read from an output file.
*/
inline unsigned int minorFromVersionBinary( unsigned int version ){
		return (( version & minorMaskBinary() ) >> 8 );
}


/*! All keywords used in c6b climate data files (in the current version). */
enum C6BKeywords {
	KW_COUNTRY,
	KW_CITY,
	KW_WMO,
	KW_SOURCE,
	KW_TIMEZONE,
	KW_LATITUDE,
	KW_LONGITUDE,
	KW_STARTYEAR,
	KW_ELEVATION,
	KW_COMMENT,
	NUM_KW
};

/*! Strings used in c6b climate file headers. */
const char * const C6B_KEYWORDS[NUM_KW] = {
	"COUNTRY",
	"CITY",
	"WMO",
	"SOURCE",
	"TIMEZONE",
	"LATITUDE",
	"LONGITUDE",
	"STARTYEAR",
	"ELEVATION",
	"COMMENT"
};

const char * const ClimateDataLoader::DEFAULT_UNITS[ClimateDataLoader::NumClimateComponents] = {
	"C",
	"%",
	"W/m2",
	"W/m2",
	"Deg",
	"m/s",
	"W/m2",
	"Pa",
	"l/m2h"
};

ClimateDataLoader::ClimateDataLoader() :
	m_longitudeInDegree(0),
	m_latitudeInDegree(0),
	m_elevation(0),
	m_timeZone(0),
	m_startYear(2000)
{
	for (unsigned int i=0; i<NumClimateComponents; ++i)
		m_currentData[i] = 0;
	m_currentData[Temperature] = 5; // to avoid 0 K as default temperature for FMU export
	m_currentData[RelativeHumidity] = 10;
}


void ClimateDataLoader::readClimateDataCCDDirectory(const IBK::Path & directory,
													SolarRadiationModel & radModel,
													std::string & skippedClimateDataFiles)
{
	const char * const FUNC_ID = "[ClimateDataLoader::readClimateDataCCDDirectory]";

	// create a list with requested input files
	std::vector<IBK::Path> dataFileNames(NumClimateComponents);
	std::vector<std::string> dataUnits(NumClimateComponents);
	// fill data files
	dataFileNames[Temperature]					= IBK::Path(directory) / "Temperature.ccd";
	dataFileNames[RelativeHumidity]				= IBK::Path(directory) / "RelativeHumidity.ccd";
	dataFileNames[DirectRadiationNormal]		= IBK::Path(directory) / "DirectRadiation.ccd";
	dataFileNames[DiffuseRadiationHorizontal]	= IBK::Path(directory) / "DiffuseRadiation.ccd";
	dataFileNames[WindDirection]				= IBK::Path(directory) / "WindDirection.ccd";
	dataFileNames[WindVelocity]					= IBK::Path(directory) / "WindVelocity.ccd";
	dataFileNames[LongWaveCounterRadiation]		= IBK::Path(directory) / "SkyRadiation.ccd";
	dataFileNames[AirPressure]					= IBK::Path(directory) / "TotalPressure.ccd";
	dataFileNames[Rain]							= IBK::Path(directory) / "VerticalRain.ccd";
	// fill data units
	for (unsigned int i=0; i<NumClimateComponents; ++i)
		dataUnits[i] = DEFAULT_UNITS[i];

	try {
		// read data files for all data
		for (unsigned int i = 0; i < NumClimateComponents; ++i) {
			try {
				std::vector<double> timeVec, dataVec;
				std::string comment, quantity;
				IBK::Unit dataUnit;
				std::string errorLine;

				ReadFunctionReturnValues res = readCCDFile(dataFileNames[i], timeVec, dataVec,
															comment, quantity, dataUnit, errorLine);

				// if not found, skip data and clear resultant vector
				if (res == RF_InvalidFilePath) {
					m_data[i] = std::vector<double>(8760, 0.0);
					skippedClimateDataFiles += dataFileNames[i].str() + "\n";
					continue;
				}
				// throw exception in case of error
				if (res != RF_OK) {
					std::string errmsg = composeErrorText(res, dataFileNames[i], errorLine);
					throw IBK::Exception( errmsg, FUNC_ID);
				}

				// let CCM do the "annual data check", time points do not exceed 365d
				// and time points 0d and 365d are not present at the same time
				checkForValidCyclicData(timeVec);

				// Let CCM insert missing hourly values
				expandToAnnualHourlyData(timeVec, dataVec);

				/// \todo check compatibility of quantities

				// convert units
				if (dataUnit != IBK::Unit(dataUnits[i])) {

					IBK::Unit target(dataUnits[i]);
					// check compatibility of units
					if (dataUnit.base_id() != target.base_id()) {
						throw IBK::Exception( IBK::FormatString("Incompatible units in file '%1'. Expected '%2', got '%3'.")
											  .arg(dataFileNames[i]).arg(dataUnits[i]).arg(dataUnit.name()), FUNC_ID);
					}

					IBK::UnitVector uvec;
					uvec.m_data.swap(dataVec); // swap in
					uvec.m_unit = dataUnit;
					uvec.convert(target);
					uvec.m_data.swap(dataVec); // swap out
				}

				// we need to convert direct radiation data into
				if (i == DirectRadiationNormal) {

					for (unsigned int k=0; k<dataVec.size(); ++k) {
						const double directRadiationHorizontal = dataVec[k];
						double directRadiationNormal;
						// calculate normal solar radiation
						radModel.convertHorizontalToNormalRadiation(timeVec[k], directRadiationHorizontal, directRadiationNormal);
						// overwrite values
						dataVec[k] = directRadiationNormal;
					}
				}

				// store data vector
				m_data[i] = dataVec;
			}
			catch (IBK::Exception & ex) {
				throw IBK::Exception(ex, IBK::FormatString("Error importing CCD file '%1'.").arg(dataFileNames[i]), FUNC_ID );
			}
		}
	}
	catch ( IBK::Exception &ex ) {
		throw IBK::Exception(ex, "Error importing CCD files from climate data directory.", FUNC_ID );
	}
}


ClimateDataLoader::ReadFunctionReturnValues ClimateDataLoader::readCCDFile(const IBK::Path & fname,
											std::vector<double> & timeInSeconds,
											std::vector<double> & dataVec,
											std::string &comment,
											std::string &quantity,
											IBK::Unit & dataUnit,
											std::string &errorLine)
{
	const char * const FUNC_ID = "[ClimateDataLoader::readCCDFile]";

#ifdef _MSC_VER
	std::ifstream in(fname.wstr().c_str());
#else // _WIN32
	std::ifstream in(fname.str().c_str());
#endif // _WIN32
	if (!in)
		return RF_InvalidFilePath; // cannot file file

	IBK::IBK_Message( IBK::FormatString("%1\n").arg(fname), IBK::MSG_PROGRESS, FUNC_ID, IBK::VL_INFO);

	unsigned int count = 0;
	std::string line;
	comment.clear();
	while (std::getline(in, line)) {
		++count;
		// skip empty lines and comments
		if (line.find_first_not_of(" \t\r") == std::string::npos) continue;
		if (line[0] == '#') {
			std::string::size_type pos = line.substr(1).find_first_not_of(" \t\r");
			if (pos != std::string::npos)
				comment += line.substr(pos+1) + "\n";
			continue;
		}
		std::stringstream linestrm(line);
		std::string dataUnitStr;
		if (!(linestrm >> quantity >> dataUnitStr)) {
			errorLine = IBK::FormatString("#%1: %2").arg(count,3).arg(line).str();
			return RF_BadHeader;
		}
		else {
			// convert string to unit
			try { dataUnit.set(dataUnitStr); }
			catch (...) {
				errorLine = IBK::FormatString("#%1: %2").arg(count,3).arg(line).str();
				return RF_BadHeader;
			}
		}
		break;
	}

	// now we have only data lines or empty lines
	timeInSeconds.clear();
	dataVec.clear();
	while (getline(in, line)) {
		++count;

		// remember original line string for error message, if any
		std::string origLine = line;

		// skip empty or comment lines
		if (line.find_first_not_of(" \t\r") == std::string::npos || line[0] == '#') continue;

		// mind that line may hold non-printable characters like spaces or tabs or \r, this is
		// handled below

		// strip trailing comments
		std::string::size_type pos = line.find('#');
		if (pos != std::string::npos) {
			line = line.substr(0, pos);
		}

		// trim trailing whitespace, space, tab and \r
		IBK::trim(line);

		// now check again for empty line
		if (line.empty()) continue;

		// substitute : with space
		replace(line.begin(), line.end(), ':', ' ');

		std::stringstream linestream(line);
		// check time point
		int day;
		int hour;
		int min;
		int sec;
		double val;
		// read and check if all values are found
		// Also check if stringstream has really read all bytes in this line, this is to
		// check against a line with content "12 1:00:00 13,21"  where val will only read 13 and stop at the ,
		if (!(linestream >> day >> hour >> min >> sec >> val) || !linestream.eof()) {
			errorLine = IBK::FormatString("#%1: %2").arg(count,3).arg(origLine).str();
			return RF_BadLineFormat;
		}

		// compute seconds of year
		double secondsOfYear = day * 24 * 3600 + hour * 3600 + min * 60 + sec;

		// check for strictly monotonic increasing time points
		if (!timeInSeconds.empty() && secondsOfYear <= timeInSeconds.back()) {
			errorLine = IBK::FormatString("#%1: %2").arg(count,3).arg(origLine).str();
			return RF_InvalidTimeStamp;
		}
		// store time point
		timeInSeconds.push_back((double) secondsOfYear);
		// store data
		dataVec.push_back(val);
	}

	// Values have been read successfully, but
	// mind that you have to check for consistency yourself:
	// - maybe no data in file at all
	// - maybe duplicate annual values (data not suitable for cyclic use)
	// - maybe invalid unit
	// - maybe unknown/invalid quantity
	return RF_OK;
}


ClimateDataLoader::ReadFunctionReturnValues ClimateDataLoader::readCSVFile(const IBK::Path & fname,
											std::vector<double> & timeInSeconds,
											std::vector<double> & dataVec,
											std::string &quantity,
											IBK::Unit & dataUnit,
											std::string &errorLine)
{
//	const char * const FUNC_ID = "[ClimateDataLoader::readCSVFile]";

	if (!fname.exists())
		return RF_InvalidFilePath; // cannot file file

	IBK::CSVReader reader;
	try {
		reader.read(fname);

		// extract time unit and value unit from caption
		if (reader.m_nColumns != 2) {
			errorLine = "";
			return RF_BadHeader;
		}

		// extract time unit for first caption
		std::string timeCaption = reader.m_captions[0];
		std::size_t p1 = timeCaption.find("[");
		std::size_t p2 = timeCaption.rfind("]");
		if (p1 == std::string::npos || p2 == std::string::npos || p1 > p2) {
			errorLine = "";
			return RF_BadHeader;
		}
		std::string timeUnitStr = timeCaption.substr(p1+1, p2-p1-1);
		IBK::trim(timeUnitStr);
		IBK::Unit timeUnit;
		try {
			timeUnit = IBK::Unit(timeUnitStr);
		}
		catch (...) {
			errorLine = timeCaption;
			return RF_BadHeader;
		}
		std::string valueCaption = reader.m_captions[1];
		p1 = valueCaption.find("[");
		p2 = valueCaption.rfind("]");
		if (p1 == std::string::npos || p2 == std::string::npos || p1 > p2) {
			errorLine = "";
			return RF_BadHeader;
		}
		quantity = valueCaption.substr(0, p1);
		IBK::trim(quantity);
		try {
			dataUnit = IBK::Unit( IBK::trim_copy(valueCaption.substr(p1+1, p2-p1-1)) );
		}
		catch (...) {
			errorLine = valueCaption;
			return RF_BadHeader;
		}

		// now extract the values
		IBK::UnitVector tv;
		tv.resize(reader.m_nRows);
		dataVec.resize(reader.m_nRows);
		for (unsigned int i=0; i<reader.m_nRows; ++i) {
			tv[i] = reader.m_values[i][0];
			dataVec[i] = reader.m_values[i][1];
		}
		// convert time vector to seconds
		tv.m_unit = timeUnit;
		tv.convert(IBK::Unit("s"));
		timeInSeconds.swap(tv.m_data);
	}
	catch (IBK::Exception & ex) {
		ex.writeMsgStackToError();
		return RF_BadLineFormat;
	}

	// Values have been read successfully, but
	// mind that you have to check for consistency yourself:
	// - maybe no data in file at all
	// - maybe duplicate annual values (data not suitable for cyclic use)
	// - maybe invalid unit
	// - maybe unknown/invalid quantity
	return RF_OK;
}


bool ClimateDataLoader::fillWMOCodeFromMLID(const IBK::Path & MLIDDatabaseFile) {

	const char * const FUNC_ID = "[ClimateDataLoader::fillWMOCodeFromMLID]";
	// code already exists
	if (!m_wmoCode.empty()) {
		IBK::IBK_Message( IBK::FormatString("Ignore request for finding WMO code inside data base %1: "
								"We defined a valid WMO already.").arg(MLIDDatabaseFile), IBK::MSG_WARNING, FUNC_ID, IBK::VL_INFO);
		return false;
	}
	// we need the country (english version) and the city
	IBK::MultiLanguageString city(m_city), country(m_country);
	std::string countryName = country.string("en");
	std::string cityName = city.string("en");

	// error: no country defined
	if (countryName.empty()) {
		IBK::IBK_Message( IBK::FormatString("Ignore request for finding WMO code inside data base %1: "
								"We need a valid country name (in english language).").arg(MLIDDatabaseFile), IBK::MSG_WARNING, FUNC_ID, IBK::VL_INFO);
		return false;
	}
	if (cityName.empty()) {
		IBK::IBK_Message( IBK::FormatString("Ignore request for finding WMO code inside data base %1: "
								"We need a valid city name (in english language).").arg(MLIDDatabaseFile), IBK::MSG_WARNING, FUNC_ID, IBK::VL_INFO);
		return false;
	}

	// open MLID file
#ifdef _MSC_VER
	std::ifstream in(MLIDDatabaseFile.wstr().c_str());
#else // _WIN32
	std::ifstream in(MLIDDatabaseFile.str().c_str());
#endif // _WIN32
	if (!in) {
		throw IBK::Exception( IBK::FormatString("%1 - not available").arg(MLIDDatabaseFile), FUNC_ID);
	}
	IBK::IBK_Message( IBK::FormatString("Reading MLID database file %1 and trying to find the current climate location %2, %3\n")
		.arg(MLIDDatabaseFile).arg(city).arg(country), IBK::MSG_PROGRESS, FUNC_ID, IBK::VL_INFO);

	// search first line in file containng the requested country
	std::string line, nextLine;
	bool countryFound = false;
	while (std::getline(in, nextLine)) {
		// skip empty lines and comments
		std::size_t found = nextLine.find(countryName);
		if ( found == std::string::npos) {
			line = nextLine;
			continue;
		}

		// check if it is really the country
		std::list<std::string> tokens;
		IBK::explode(nextLine, tokens, ',');
		// the third entry is the country
		if(tokens.size() < 3)
			throw IBK::Exception( IBK::FormatString("Invalid MLID data format in file %1.").arg(MLIDDatabaseFile), FUNC_ID);

		std::list<std::string>::iterator lineIt =
			tokens.begin();
		std::advance(lineIt,2);

		// countries name is used anywhere else
		if(lineIt->find(countryName) == std::string::npos){
			line = nextLine;
			continue;
		}
		countryFound = true;
		break;
	}

	// end of file
	if (!countryFound) {
		IBK::IBK_Message( IBK::FormatString("Ignore request for finding WMO code inside data base %1: "
								"Country name %2 is unknown.")
								.arg(MLIDDatabaseFile).arg(countryName), IBK::MSG_WARNING,
								FUNC_ID, IBK::VL_INFO);
		return false;
	}

	bool cityFound = false;
	// search city
	while (std::getline(in, line)) {
		// new country: break
		std::size_t found = line.find(countryName);
		if ( found == std::string::npos) {
			break;
		}
		found = line.find(cityName);
		if(found == std::string::npos) {
			continue;
		}
		// check if it is really the city
		std::list<std::string> tokens;
		IBK::explode(line, tokens, ',');
		// the 6th entry is the country
		if (tokens.size() < 31)
			throw IBK::Exception( IBK::FormatString("Invalid MLID data format in file %1.").arg(MLIDDatabaseFile), FUNC_ID);

		std::list<std::string>::iterator lineIt =
			tokens.begin();
		std::advance(lineIt,5);

		// city's name is used anywhere else
		if (lineIt->find(cityName) == std::string::npos)
			continue;

		// we have the correct city
		cityFound = true;
		// now check the location (longitude and latitude)
		// latitude is the 30th entry, longitude the 31th
		lineIt = tokens.begin();
		std::advance(lineIt,29);
		try {
			double latitude = IBK::string2val<double>(*lineIt);
			std::advance(lineIt,1);
			double longitude = IBK::string2val<double>(*lineIt);

			// we are close enough to the requested location
			if (std::fabs(latitude - m_latitudeInDegree) < 0.1 &&
				std::fabs(longitude - m_longitudeInDegree) < 0.1)
			{
				// the 25th token is the maslib identifier (we use as WMO)
				lineIt = tokens.begin();
				std::advance(lineIt,24);
				m_wmoCode = *lineIt;
				// trim code
				IBK::trim(m_wmoCode);
				return true;
			}
		}
		catch (IBK::Exception &ex) {
			throw IBK::Exception( ex, IBK::FormatString("Invalid MLID data format in file %1.").arg(MLIDDatabaseFile), FUNC_ID);
		}
	}

	// end of country section
	if (!cityFound) {
		IBK::IBK_Message( IBK::FormatString("Ignore request for finding WMO code inside data base %1: "
								"City name %2 is unknown.")
								.arg(MLIDDatabaseFile).arg(cityName), IBK::MSG_WARNING,
								FUNC_ID, IBK::VL_INFO);
		return false;
	}
	// location was not found
	IBK::IBK_Message( IBK::FormatString("Ignore request for finding WMO code inside data base %1: "
							"Location with latitude %2 and longitude %3 is unknown.")
							.arg(MLIDDatabaseFile).arg(m_latitudeInDegree).arg(m_longitudeInDegree), IBK::MSG_WARNING,
							FUNC_ID, IBK::VL_INFO);
	return false;
}


void ClimateDataLoader::readClimateData(const IBK::Path & fname, bool headerOnly) {
	const char * const FUNC_ID = "[ClimateDataLoader::readClimateData]";
	std::string ext = fname.extension();
	if (IBK::string_nocase_compare(ext,"epw"))
		readClimateDataEPW(fname, headerOnly);
	// only binary reading is enabled at the moment
	else if (IBK::string_nocase_compare(ext,"c6b"))
		readClimateDataIBK(fname, headerOnly);
	else if (IBK::string_nocase_compare(ext,"wac"))
		readClimateDataWAC(fname, headerOnly);
	else
		throw IBK::Exception("Unknown/unsupported climate data file format.", FUNC_ID);
}


void ClimateDataLoader::readClimateDataEPW(const IBK::Path & fname, bool headerOnly) {
	const char * const FUNC_ID = "[ClimateDataLoader::readClimateDataEPW]";
	// open file
#ifdef _MSC_VER
	std::ifstream in(fname.wstr().c_str());
#else
	std::ifstream in(fname.str().c_str());
#endif
	// check if file exists
	if (!in)
		throw IBK::Exception( IBK::FormatString("Climate data file '%1' does not exist or is not accessible.")
							  .arg(fname), FUNC_ID);

	try {
		// read first header line
		std::string line;
		if (!getline(in, line))
			throw IBK::Exception( IBK::FormatString("Error reading line 1."), FUNC_ID);
		// separate by comma
		std::vector<std::string> tokens;
		IBK::trim(line);
		IBK::explode_csv(line, tokens);
		if (tokens.size() < 10)
			throw IBK::Exception( IBK::FormatString("Error reading line 1, bad format."), FUNC_ID);

		m_city = tokens[1];
		m_country = tokens[3];
		m_source = tokens[4];
		m_wmoCode = tokens[5];
		m_latitudeInDegree = IBK::string2val<double>(tokens[6]);
		m_longitudeInDegree = IBK::string2val<double>(tokens[7]);
		m_timeZone = (int)IBK::string2val<double>(tokens[8]);
		m_elevation = IBK::string2val<double>(tokens[9]);

		// skip remaining header lines
		getline(in, line);
		getline(in, line);
		getline(in, line);
		getline(in, line);
		getline(in, line); // first comment line
		IBK::trim(line);
		IBK::explode_csv(line, tokens);
		if (tokens.size() == 2) {
			IBK::trim(tokens[1]);
			m_comment = tokens[1];
		}
		getline(in, line); // second comment line
		IBK::trim(line);
		IBK::explode_csv(line, tokens);
		if (tokens.size() == 2) {
			IBK::trim(tokens[1]);
			if (!tokens[1].empty())
				m_comment += '\n' + tokens[1];
		}

		getline(in, line);


		if (headerOnly)
			return;


		// remove existing data
		for (int i=0; i<NumClimateComponents; ++i)
			m_data[i].clear();

		// now read all 8760 hours of the year
		for (unsigned int i=0; i<8760; ++i) {
			getline(in, line);
			IBK::trim(line);
			std::vector<std::string> tokens;
			IBK::explode_csv(line, tokens);
			m_data[Temperature].push_back( IBK::string2val<double>(tokens[6]));
			m_data[RelativeHumidity].push_back( IBK::string2val<double>(tokens[8]));
			m_data[DirectRadiationNormal].push_back( IBK::string2val<double>(tokens[14]) );
			m_data[DiffuseRadiationHorizontal].push_back( IBK::string2val<double>(tokens[15]) );
			m_data[WindDirection].push_back( IBK::string2val<double>(tokens[20]));
			m_data[WindVelocity].push_back( IBK::string2val<double>(tokens[21]) );
			m_data[LongWaveCounterRadiation].push_back( IBK::string2val<double>(tokens[12]) );
			m_data[AirPressure].push_back( IBK::string2val<double>(tokens[9]) ); // already in Pa
		}
		// no rain data in epw
		m_data[Rain].resize(8760);
		std::fill(m_data[Rain].begin(), m_data[Rain].end(), 0.0);
	}
	catch (IBK::Exception & ex) {
		throw IBK::Exception(ex, IBK::FormatString("Error reading file '%1'.").arg(fname), FUNC_ID);
	}
}


void ClimateDataLoader::readClimateDataIBK(const IBK::Path & fname, bool headerOnly) {
	const char * const FUNC_ID = "[ClimateDataLoader::readClimateDataIBK]";
	// open file in binary mode
#ifdef _MSC_VER
	std::ifstream in(fname.wstr().c_str(), std::ios_base::binary);
#else
	std::ifstream in(fname.str().c_str(), std::ios_base::binary);
#endif
	// check if file exists
	if (!in)
		throw IBK::Exception( IBK::FormatString("Climate data file '%1' does not exist or is not accessible.")
							  .arg(fname), FUNC_ID);

	// read magic header and determine file format and version
	unsigned int test = 0;

	// read first unsigned int from stream
	IBK::read_uint32_binary( in, test);

	// check if this is a binary climate
	if (test != MAGIC_FIRST_NUMBER)
		throw IBK::Exception("Wrong first magic number!", FUNC_ID);

	// read second unsigned int from stream
	IBK::read_uint32_binary( in, test);

	// test magic header
	if ( test != MAGIC_SECOND_NUMBER)
		throw IBK::Exception("Wrong second magic number!", FUNC_ID);

	// read file version and split it
	IBK::read_uint32_binary( in, test);
	unsigned int majorFileVersion = majorFromVersionBinary( test );
	unsigned int minorFileVersion = minorFromVersionBinary( test );
	(void)minorFileVersion;

	if (majorFileVersion != 1)
		throw IBK::Exception("Unsupported file version.", FUNC_ID);
	unsigned int dummy;
	// read last unsigned integer
	IBK::read_uint32_binary( in, dummy);
	IBK_ASSERT(dummy == 0); // safety check

	// read number of keyword strings
	IBK::read_uint32_binary( in, test);

	for (unsigned int i=0; i<test; ++i) {

		std::string keyval;

		IBK::read_string_binary(in, keyval, 2000);

		// extract keyword from string
		std::vector< std::string > parts;
		size_t count = IBK::explode( keyval, parts, "=", IBK::EF_TrimTokens | IBK::EF_KeepEmptyTokens);

		if (count == 1) {
			if(parts[0] == C6B_KEYWORDS[KW_COMMENT]) {
				m_comment.clear();
			}
			else {
				throw IBK::Exception("Invalid key-value format.", FUNC_ID);
			}
		}
		else {
			if (count != 2) {
				throw IBK::Exception("Invalid key-value format.", FUNC_ID);
			}
		}

		if ( parts[0] == C6B_KEYWORDS[KW_COUNTRY] )
			m_country = parts[1];
		else if ( parts[0] == C6B_KEYWORDS[KW_CITY] )
			m_city = parts[1];
		else if ( parts[0] == C6B_KEYWORDS[KW_WMO] )
			m_wmoCode = parts[1];
		else if ( parts[0] == C6B_KEYWORDS[KW_SOURCE] )
			m_source = parts[1];
		else if ( parts[0] == C6B_KEYWORDS[KW_TIMEZONE] )
			m_timeZone = (int)IBK::string2val<double>(parts[1]);
		else if ( parts[0] == C6B_KEYWORDS[KW_LATITUDE] )
			m_latitudeInDegree = IBK::string2val<double>(parts[1]);
		else if ( parts[0] == C6B_KEYWORDS[KW_LONGITUDE] )
			m_longitudeInDegree = IBK::string2val<double>(parts[1]);
		else if ( parts[0] == C6B_KEYWORDS[KW_STARTYEAR] )
			m_startYear = IBK::string2val<int>(parts[1]);
		else if ( parts[0] == C6B_KEYWORDS[KW_ELEVATION] )
			m_elevation = IBK::string2val<int>(parts[1]);
		else if ( parts[0] == C6B_KEYWORDS[KW_COMMENT] && count == 2)
			m_comment = parts[1];

	}

	// check for completness
	if (m_city.empty()) {
		IBK::IBK_Message("Missing CITY name.", IBK::MSG_WARNING, FUNC_ID);
		m_city = "---";
	}

	if (headerOnly)
		return;

	unsigned int dataLen = 0;
	for (unsigned int c=0; c<NumClimateComponents; ++c) {
		IBK::read_vector_binary(in, m_data[c], 87600); // limit set to 10 years of data
		if (c==0)
			dataLen = (unsigned int)m_data[c].size();
		else if (dataLen != m_data[c].size())
			throw IBK::Exception(IBK::FormatString("Error reading data file, data vector #%1 has "
												   "length %2, but should have length %3 as all previous vectors.")
								 .arg(c).arg((unsigned int) m_data[c].size()).arg(dataLen), FUNC_ID);
	}

	IBK::read_vector_binary(in, m_dataTimePoints, 87600);
}


void ClimateDataLoader::readClimateDataWAC(const IBK::Path & fname, bool headerOnly) {
	const char * const FUNC_ID = "[ClimateDataLoader::readClimateDataWAC]";
	// open file
#ifdef _MSC_VER
	std::ifstream in(fname.wstr().c_str());
#else
	std::ifstream in(fname.str().c_str());
#endif
	// check if file exists
	if (!in)
		throw IBK::Exception( IBK::FormatString("Climate data file '%1' does not exist or is not accessible.")
							  .arg(fname), FUNC_ID);

	try {
		// read first header line
		std::string line;
		if (!std::getline(in, line))
			throw IBK::Exception( IBK::FormatString("Error reading line 1."), FUNC_ID);
		// check for correct file
		if (line.find("WAC") == std::string::npos)
			throw IBK::Exception( IBK::FormatString("Invalid file format, expected WAC in line 1."), FUNC_ID);
		// second line
		if (!std::getline(in, line))
			throw IBK::Exception( IBK::FormatString("Error reading line 1."), FUNC_ID);
		// read number of header lines
		std::stringstream lstrm(line);
		unsigned int headerLineCount = 0;
		if (!(lstrm >> headerLineCount))
			throw IBK::Exception( IBK::FormatString("Error reading number of header lines from line 2."), FUNC_ID);
		// read remaining header lines
		std::vector<std::string> headerLines;
		for (unsigned int i=2; i<=headerLineCount; ++i) {
			if (!std::getline(in, line))
				throw IBK::Exception( IBK::FormatString("Expected header lines (offset) %1, but got an error reading line #%2.")
									  .arg(headerLineCount).arg(i+1), FUNC_ID);
			headerLines.push_back(line);
		}

		// parse header
		if (headerLineCount < 9)
			throw IBK::Exception( IBK::FormatString("Missing header lines, expected at least 9 lines."), FUNC_ID);

		m_city = IBK::trim_copy(headerLines[0]);
		m_comment = headerLines[1];
		m_longitudeInDegree = IBK::string2val<double>(headerLines[2]);
		m_latitudeInDegree = IBK::string2val<double>(headerLines[3]);
		m_elevation = IBK::string2val<double>(headerLines[4]);
		m_timeZone = (int)IBK::string2val<double>(headerLines[5]);
		if (m_timeZone > 12)
			m_timeZone -= 24;
		double timeStep = IBK::string2val<double>(headerLines[6]);
		unsigned int numLines = IBK::string2val<unsigned int>(headerLines[7]);
		unsigned int numColumns = IBK::string2val<unsigned int>(headerLines[8]);

		if (!std::getline(in, line))
			throw IBK::Exception( IBK::FormatString("Error reading last header line with table captions."), FUNC_ID);

		// compose mapping of table captions to column indexes

		std::vector<int> columnIndexes(NumClimateComponents, -1);

		int indexGlobalRad = -1;
		int timeColumnIndex = -1;
		std::vector<std::string> columnCaptions;
		IBK::explode(line, columnCaptions, "\t", IBK::EF_TrimTokens);
		for (unsigned int i=0; i<columnCaptions.size(); ++i) {
			if (columnCaptions[i] == "TA")
				columnIndexes[Temperature] = i;
			else if (columnCaptions[i] == "HREL")
				columnIndexes[RelativeHumidity] = i;
			else if (columnCaptions[i] == "ISGH")
				indexGlobalRad = i; // conversion follows after reading
			else if (columnCaptions[i] == "ISDH")
				columnIndexes[DirectRadiationNormal] = i; // conversion follows after reading
			else if (columnCaptions[i] == "ISD")
				columnIndexes[DiffuseRadiationHorizontal] = i;
			else if (columnCaptions[i] == "WS")
				columnIndexes[WindVelocity] = i;
			else if (columnCaptions[i] == "WV")
				columnIndexes[WindVelocity] = i;
			else if (columnCaptions[i] == "WD")
				columnIndexes[WindDirection] = i;
			else if (columnCaptions[i] == "RN")
				columnIndexes[Rain] = i;
			else if (columnCaptions[i] == "ILAH")
				columnIndexes[LongWaveCounterRadiation] = i;
			else if (columnCaptions[i] == "PSTA")
				columnIndexes[AirPressure] = i;
			else if (columnCaptions[i] == "TIME")
				timeColumnIndex = i;
		}

		// if time column is missing, we must have exactly 8760 values
		if (timeColumnIndex == -1) {
			if (numLines != 8760 && timeStep != 1) {
				throw IBK::Exception("WAC files without time column must be annual climate data files "
									 "with 8760 values spaced at 1 h time difference.", FUNC_ID);
			}
		}

		// global radiation instead of direct radiation?
		if (indexGlobalRad != -1) {
			if (columnIndexes[DiffuseRadiationHorizontal] == -1)
				throw IBK::Exception("Diffuse radiation is required to split global radiation into direct radiation component!", FUNC_ID);
			// first read in global radiation in direct radiation slot, then subtract diffuse radiation
			// so first store index in direct radiation slot
			columnIndexes[DirectRadiationNormal] = indexGlobalRad;
		}
		else {
			if (columnIndexes[DirectRadiationNormal] == -1)
				throw IBK::Exception("Missing direct radiation or global radiation, either must be present!", FUNC_ID);
		}

		int biggestIndex = -1;
		for (unsigned int i=0; i<NumClimateComponents; ++i) {
			biggestIndex = std::max<int>(biggestIndex, columnIndexes[i]);
		}

		if (headerOnly)
			return;

		// remove existing data
		for (int i=0; i<NumClimateComponents; ++i) {
			m_data[i].clear();
			m_data[i].reserve(numLines);
		}
		m_dataTimePoints.clear();

		std::string lastLine = line;
		// distingish between cyclic or continuous data
		int startHour = -1;
		for (unsigned int i=0; i<numLines; ++i) {

			if (!std::getline(in, line))
				throw IBK::Exception( IBK::FormatString("Error reading data line #%1 (total line #%2), last successfully read line was '%3'.")
									  .arg(i+1).arg(i+headerLines.size()+2).arg(lastLine), FUNC_ID);

			IBK::trim(line);
			std::vector<std::string> tokens;
			IBK::explode(line, tokens, "\t", IBK::EF_NoFlags);

			if (tokens.size() != numColumns)
				throw IBK::Exception( IBK::FormatString("Error reading data line #%1 (total line #%2); not enough columns "
														"(expected %3 columns, got %4 columns)!")
									  .arg(i+1).arg(i+headerLines.size()+2)
									  .arg(numColumns).arg(tokens.size()), FUNC_ID);

			// different handling depending on annual hourly cyclic data or continuous data
			if (timeStep == 1 && numLines == 8760) {
				// cyclic data
				if (startHour == -1) {
					if (timeColumnIndex != -1) {
						std::string timeStr = tokens[timeColumnIndex];
						timeStr = IBK::replace_string(timeStr, ":", " ");
						timeStr = IBK::replace_string(timeStr, "-", " ");
						std::stringstream lineStream(timeStr);
						int y,m,d;
						if (!(lineStream >> y >> m >> d >> startHour))
							throw IBK::Exception( IBK::FormatString("Error parsing date/time entry in line #%1 (total line #%2)")
												  .arg(i+1).arg(i+headerLines.size()+2), FUNC_ID);
						m_startYear = y;
					}
					else {
						// no time column, set start year to something generic
						m_startYear = 2007;
					}
				}
			}
			else {
				// continuous data, compute time point
				std::string timeStr = tokens[timeColumnIndex];
				timeStr = IBK::replace_string(timeStr, ":", " ");
				timeStr = IBK::replace_string(timeStr, "-", " ");
				std::stringstream lineStream(timeStr);
				int y,m,d,h,minutes;
				if (!(lineStream >> y >> m >> d >> h >> minutes))
					throw IBK::Exception( IBK::FormatString("Error parsing date/time entry in line #%1 (total line #%2)")
										  .arg(i+1).arg(i+headerLines.size()+2), FUNC_ID);
				if (startHour == -1) {
					m_startYear = y;
					startHour = h-1;
				}
				// mind that day and month are given with 1 offset
				--d;
				--m;
				// h goes from 1..24, we first subtract one hour
				--h;
				IBK::Time dt(y, m, d, h*3600 + minutes*60);
				// then add 3600 seconds again
				dt += 3600;
				IBK::Time dtStart(m_startYear, 0, 0, 0);
				dt -= dtStart;
				m_dataTimePoints.push_back(dt.secondsOfYear()+dt.year()*SECONDS_PER_YEAR);
			}

			// for now assume fixed column matching
			m_data[Temperature].push_back( IBK::string2val<double>(tokens[columnIndexes[Temperature]]));
			m_data[RelativeHumidity].push_back( IBK::string2val<double>(tokens[columnIndexes[RelativeHumidity]])*100);
			m_data[DirectRadiationNormal].push_back( IBK::string2val<double>(tokens[columnIndexes[DirectRadiationNormal]]) ); // mind: still horizontal radiation, conversion follows
			m_data[DiffuseRadiationHorizontal].push_back( IBK::string2val<double>(tokens[columnIndexes[DiffuseRadiationHorizontal]]) );

			// if global radiation is given, compute direct radiation
			if (indexGlobalRad != -1) {
				m_data[DirectRadiationNormal].back() -= m_data[DiffuseRadiationHorizontal].back();
				if (m_data[DirectRadiationNormal].back() < 0)
					throw IBK::Exception( IBK::FormatString("Error in line #%1 (total line #%2): global radiation intensity must not be smaller than diffuse radiation intensity!")
										  .arg(i+1).arg(i+headerLines.size()+2), FUNC_ID);
			}

			if (columnIndexes[Rain] != -1)
				m_data[Rain].push_back( IBK::string2val<double>(tokens[columnIndexes[Rain]]) );
			else
				m_data[Rain].push_back(0);

			if (columnIndexes[WindDirection] != -1)
				m_data[WindDirection].push_back( IBK::string2val<double>(tokens[columnIndexes[WindDirection]]));
			else
				m_data[WindDirection].push_back(0);

			if (columnIndexes[WindVelocity] != -1)
				m_data[WindVelocity].push_back( IBK::string2val<double>(tokens[columnIndexes[WindVelocity]]) );
			else
				m_data[WindVelocity].push_back(0);

			if (columnIndexes[LongWaveCounterRadiation] != -1)
				m_data[LongWaveCounterRadiation].push_back( IBK::string2val<double>(tokens[columnIndexes[LongWaveCounterRadiation]]) );
			else
				m_data[LongWaveCounterRadiation].push_back(0);

			if (columnIndexes[AirPressure] != -1)
				m_data[AirPressure].push_back( IBK::string2val<double>(tokens[columnIndexes[AirPressure]])*100 ); // converted from hPa
			else
				m_data[AirPressure].push_back(0);
		} // while

		// now convert horizontal radiation to normal radiation
		CCM::SolarRadiationModel radModel;
		radModel.m_sunPositionModel.m_latitude = m_latitudeInDegree * DEG2RAD;
		radModel.m_sunPositionModel.m_longitude = m_longitudeInDegree * DEG2RAD;
		radModel.m_climateDataLoader.m_timeZone = m_timeZone;

		std::vector<double> & dataVec = m_data[DirectRadiationNormal];
		IBK_ASSERT(m_dataTimePoints.empty() || dataVec.size() == m_dataTimePoints.size());
		for (unsigned int k=0; k<dataVec.size(); ++k) {
			const double directRadiationHorizontal = dataVec[k];
			double directRadiationNormal;
			double secondsOfYear = (k+1)*3600; // assume cyclic data
			if (!m_dataTimePoints.empty())
				secondsOfYear = m_dataTimePoints[k];

			// calculate normal solar radiation
			radModel.convertHorizontalToNormalRadiation(secondsOfYear, directRadiationHorizontal, directRadiationNormal);
			// overwrite values
			dataVec[k] = directRadiationNormal;
		}

	}
	catch (IBK::Exception & ex) {
		throw IBK::Exception(ex, IBK::FormatString("Error reading file '%1'.").arg(fname), FUNC_ID);
	}
}


void ClimateDataLoader::writeClimateDataIBK(const IBK::Path & fname) {
	//const char * const FUNC_ID = "[ClimateDataLoader::writeClimateDataIBK]";
	// open file in binary mode
#ifdef _MSC_VER
	std::ofstream out(fname.wstr().c_str(), std::ios_base::binary);
#else
	std::ofstream out(fname.str().c_str(), std::ios_base::binary);
#endif
	// write magic header and version number
	IBK::write_uint32_binary( out, MAGIC_FIRST_NUMBER );
	IBK::write_uint32_binary( out, MAGIC_SECOND_NUMBER );
	IBK::write_uint32_binary( out, versionNumberBinary() );
	IBK::write_uint32_binary( out, 0 );

	// write meta data

	// write binary data
	unsigned int keywordCount = NUM_KW;
	IBK::write_uint32_binary(out, keywordCount);

	IBK::write_string_binary(out, std::string(C6B_KEYWORDS[KW_COUNTRY]) + "=" + m_country);
	IBK::write_string_binary(out, std::string(C6B_KEYWORDS[KW_CITY]) + "="+m_city);
	IBK::write_string_binary(out, std::string(C6B_KEYWORDS[KW_WMO]) + "="+m_wmoCode);
	IBK::write_string_binary(out, std::string(C6B_KEYWORDS[KW_SOURCE]) + "="+m_source);
	IBK::write_string_binary(out, std::string(C6B_KEYWORDS[KW_TIMEZONE]) + "="+IBK::val2string(m_timeZone));
	IBK::write_string_binary(out, std::string(C6B_KEYWORDS[KW_LATITUDE]) + "="+IBK::val2string(m_latitudeInDegree));
	IBK::write_string_binary(out, std::string(C6B_KEYWORDS[KW_LONGITUDE]) + "="+IBK::val2string(m_longitudeInDegree));
	IBK::write_string_binary(out, std::string(C6B_KEYWORDS[KW_STARTYEAR]) + "="+IBK::val2string(m_startYear));
	IBK::write_string_binary(out, std::string(C6B_KEYWORDS[KW_ELEVATION]) + "="+IBK::val2string(m_elevation));
	IBK::write_string_binary(out, std::string(C6B_KEYWORDS[KW_COMMENT]) + "="+m_comment);

	for (unsigned int c=0; c<NumClimateComponents; ++c) {
		IBK::write_vector_binary(out, m_data[c]);
	}

	IBK::write_vector_binary(out, m_dataTimePoints);
}


void ClimateDataLoader::writeClimateDataTSV(const IBK::Path & fname) {
#ifdef _MSC_VER
	std::ofstream out(fname.wstr().c_str());
#else
	std::ofstream out(fname.str().c_str());
#endif
	if (!m_dataTimePoints.empty()) {
		out << "Time [d]\t"
			<< "Temperature [C]\t"
			<< "Relative Humidity [%]\t"
			<< "DirectRadiationNormal [W/m2]\t"
			<< "DiffuseRadiationHorizontal [W/m2]\t"
			<< "WindDirection [deg]\t"
			<< "WindVelocity [m/s]\t"
			<< "LongWaveCounterRadiation [W/m2]"
			<< "AirPressure [Pa]"
			<< "Rain [l/m2h]"
			<< "\n";

		for (unsigned int i=0; i<8760; ++i) {
			out << m_dataTimePoints[i]/(24*3600) << "\t";
			for (unsigned int j=0; j<NumClimateComponents; ++j)
				out << m_data[j][i] << "\t";
			out << std::endl;
		}
	}
	else {
		// first column is hour of year
		// Write header
		out << "Hour of year\t"
			<< "Date/Time\t"
			<< "Temperature [C]\t"
			<< "Relative Humidity [%]\t"
			<< "DirectRadiationNormal [W/m2]\t"
			<< "DiffuseRadiationHorizontal [W/m2]\t"
			<< "WindDirection [deg]\t"
			<< "WindVelocity [m/s]\t"
			<< "LongWaveCounterRadiation [W/m2]"
			<< "AirPressure [Pa]"
			<< "Rain [l/m2h]"
			<< "\n";

		for (unsigned int i=0; i<8760; ++i) {
			out << i+1 << "\t";
			out << IBK::Time(2000, (i+1)*3600).toShortDateFormat() << "\t";
			for (unsigned int j=0; j<NumClimateComponents; ++j)
				out << m_data[j][i] << "\t";
			out << std::endl;
		}
	}
}


void ClimateDataLoader::readDescriptionCCD(const IBK::Path &fname) {

	const char * const FUNC_ID = "[ClimateDataLoader::readDescriptionCCD]";
	// create document
	TiXmlDocument doc;

	// correctness check of file name
	if ( !fname.isFile() )
		throw IBK::Exception(IBK::FormatString("File '%1' does not exist or cannot be opened for reading.")
				.arg(fname), FUNC_ID);

#if defined(_WIN32)
	std::string filename = IBK::WstringToANSI(IBK::UTF8ToWstring(fname.str()), false);
#else
	std::string filename = fname.str();
#endif


	if (!doc.LoadFile(filename.c_str(), TIXML_ENCODING_UTF8)) {
		throw IBK::Exception(IBK::FormatString("Error in line %1 of project file '%2':\n%3")
				.arg(doc.ErrorRow())
				.arg(fname)
				.arg(doc.ErrorDesc()), FUNC_ID);
	}

	// we use a handle so that NULL pointer checks are done during the query functions
	TiXmlHandle xmlHandleDoc(&doc);

	// read root element
	TiXmlElement * xmlElem = xmlHandleDoc.FirstChildElement().Element();
	if (!xmlElem) {
		throw IBK::Exception( IBK::FormatString("%1 - not available").arg(fname), FUNC_ID);
	}
	// retreive root node
	std::string rootnode = xmlElem->Value();
	if (rootnode != "ClimateDataLocation")
		throw IBK::Exception( IBK::FormatString("Expected 'ClimateDataLocation' as root node in description XML file %1.")
							.arg(fname), FUNC_ID);
	// we read our subsections from this handle
	TiXmlHandle xmlRoot = TiXmlHandle(xmlElem);

	// Latitude and longitude
	xmlElem = xmlRoot.FirstChild( "Position" ).Element();
	if(!xmlElem) {
		throw IBK::Exception( IBK::FormatString("Expected tag 'Position' in description XML file %1.")
						.arg(fname), FUNC_ID);
	}

	// read latitude and longitude from attributes
	const TiXmlAttribute * attrib = TiXmlAttribute::attributeByName(xmlElem, "latitude");
	if (attrib == NULL){
		throw IBK::Exception( IBK::FormatString("Expected attribute 'latitude' in 'Position' tag in description XML file %1.")
						.arg(fname), FUNC_ID);
	}
	m_latitudeInDegree = IBK::string2val<double>(attrib->Value());

	attrib = TiXmlAttribute::attributeByName(xmlElem, "longitude");
	if (attrib == NULL){
		throw IBK::Exception( IBK::FormatString("Expected attribute 'longitude' in 'Position' tag in description XML file %1.")
						.arg(fname), FUNC_ID);
	}
	m_longitudeInDegree = IBK::string2val<double>(attrib->Value());

	attrib = TiXmlAttribute::attributeByName(xmlElem, "altitude");
	if (attrib == NULL) {
		IBK::IBK_Message( IBK::FormatString("Expected attribute 'altitude' in 'Position' tag in description XML file %1.").arg(fname),
						  IBK::MSG_WARNING, FUNC_ID);
		m_elevation = 0;
	}
	else
		m_elevation = IBK::string2val<double>(attrib->Value());

	// Time zone
	xmlElem = xmlRoot.FirstChild( "TimeZone" ).Element();
	if (!xmlElem) {
		IBK::IBK_Message( IBK::FormatString("Expected tag 'TimeZone' in description XML file %1.").arg(fname), IBK::MSG_WARNING, FUNC_ID);
		m_timeZone = 1;
	}
	else
		m_timeZone = IBK::string2val<int>(xmlElem->GetText());

	// compose a multi languiage string for city and country
	IBK::MultiLanguageString city, country;

	// read city and country from description: first city, than country
	xmlElem = xmlRoot.FirstChild( "Description" ).Element();
	if(!xmlElem) {
		throw IBK::Exception( IBK::FormatString("Expected tag 'Description' in description XML file %1.")
						.arg(fname), FUNC_ID);
	}

	unsigned int childIdx = 0;
	// find english expression
	while(xmlElem != NULL) {
		// search for english decription
		attrib = TiXmlAttribute::attributeByName(xmlElem, "langid");
		if (attrib == NULL){
			throw IBK::Exception( IBK::FormatString("Expected attribute 'langid' in 'Description' tag in description XML file %1.")
							.arg(fname), FUNC_ID);
		}
		std::string langID = attrib->Value();
		// split into city and country
		std::string locationString = xmlElem->GetText();

		// split via , and trim tokens
		std::list<std::string> tokens;
		IBK::explode(locationString, tokens,',');
		// format error
		if(tokens.size() < 2) {
			throw IBK::Exception( IBK::FormatString("Expected city and country inside 'Description' tag in description XML file %1.")
							.arg(fname), FUNC_ID);
		}

		std::string cityName = *tokens.begin();
		IBK::trim(cityName);
		// store city name in the current language
		city.setString( cityName, langID);

		// fill country
		std::list<std::string>::iterator countryIt = tokens.begin();
		std::advance(countryIt,1);
		std::string countryName = *countryIt;
		IBK::trim(countryName);
		// store country name in the current language
		country.setString( countryName, langID);

		++childIdx;
		xmlElem = xmlRoot.Child( "Description", childIdx).Element();
	}
	// copy city and country string
	m_city = city.encodedString();
	m_country = country.encodedString();
	// set a source
	xmlElem = xmlRoot.FirstChild( "Source" ).Element();
	if (!xmlElem) {
		m_source = fname.parentPath().str();
	}
	else
		m_source = xmlElem->GetText();
	// set a source
	xmlElem = xmlRoot.FirstChild( "Comment" ).Element();
	if (!xmlElem) {
		m_comment = "Data converted from CCD format";
	}
	else
		m_comment = xmlElem->GetText();
}


void ClimateDataLoader::setTime(int year, double secondsOfYear) {
	const char * const FUNC_ID = "[ClimateDataLoader::setTime]";

	// if no data set was provided by the user we abort immediately
	if (m_city.empty())
		throw IBK::Exception("Climata data loader not properly initialized, at least City property is missing.",
							 FUNC_ID);

	double t = secondsOfYear;

	// Note: Values are interpreted as given at the end of each hour.
	//       For the first hour, we interpolate between last value and first value in data table.

	// First compute first and second hour index and interpolation factor for table data

	double alpha; // 1 = use value at hourIndex1, 0 = use value at hourIndex2
	unsigned int hourIndex1, hourIndex2;

	// equi-distant hourly data?
	if (m_dataTimePoints.empty()) {
		// normalize to year
		while (t >= SECONDS_PER_YEAR)
			t -= (double)SECONDS_PER_YEAR;
		// if within first hour, use average between value at last hour and first hour
		if (t < 3600) {
			alpha = 1 - t/3600;
			hourIndex1 = 8759; // hour 8760 = hour 0
			hourIndex2 = 0; // hour 1
		}
		else {
			hourIndex1 = (unsigned int) (std::floor(t/3600)-1); // mind: t = 3600 -> hourIndex1 = 0; hourIndex1 max 8758
			hourIndex2 = hourIndex1 + 1;
			alpha = 1 - (t - hourIndex2*3600)/3600;
		}
	}
	else {
		if (m_dataTimePoints.size() != m_data[Temperature].size())
			throw IBK::Exception("Mismatching sizes of time points vector and data vectors (non-cyclic data).", FUNC_ID);

		// for each year that we differ from startYear, add appropriate number of seconds
		t += (double) SECONDS_PER_YEAR * (year - m_startYear);
		// ensure, that time point is within interval spanned by m_dataTimePoints
		if (t < m_dataTimePoints.front() || t > m_dataTimePoints.back())
			throw IBK::Exception( IBK::FormatString("Time point out of range of time points vector (%1;%2), data available "
													"for interval %3 - %4, start year of simulation %5, start year of climate data %6.")
								  .arg(year)
								  .arg(IBK::Time::format_time_difference(secondsOfYear))
								  .arg(IBK::Time::format_time_difference(m_dataTimePoints.front()))
								  .arg(IBK::Time::format_time_difference(m_dataTimePoints.back()))
								  .arg(year).arg(m_startYear), FUNC_ID);

		// lookup t in m_dataTimePoints
		std::vector<double>::const_iterator it = std::lower_bound(m_dataTimePoints.begin(), m_dataTimePoints.end(), t);
		// it == m_dataTimePoints.begin() : t <= m_dataTimePoints[0]
		// it == m_dataTimePoints.end() : t > m_dataTimePoints.back()
		// it == m_dataTimePoints.begin()+1 : m_dataTimePoints[0] < t <= m_dataTimePoints[1]
		if (it == m_dataTimePoints.begin()) {
			hourIndex2 = hourIndex1 = 0;
			alpha = 1;
		}
		else if (it == m_dataTimePoints.end()) {
			hourIndex2 = hourIndex1 = (unsigned int)m_dataTimePoints.size()-1;
			alpha = 1;
		}
		else {
			// 64bit -> 32bit cast, hope we don't have gigabytes of data... :-)
			hourIndex2 = static_cast<unsigned int>(it - m_dataTimePoints.begin());
			hourIndex1 = hourIndex2-1;
			alpha = 1 - (t - m_dataTimePoints[hourIndex1])/(m_dataTimePoints[hourIndex2]-m_dataTimePoints[hourIndex1]);
		}
	}

	// now compute interpolated values for each component
	for (unsigned int c=0; c<NumClimateComponents; ++c) {
		// no override parameter given?
		if (m_overrideData[c].empty()) {
			IBK_ASSERT_XX(m_data[c].size() > std::max(hourIndex1, hourIndex2),
						  IBK::FormatString("#CC=%1, hourIndex1=%2, hourIndex2=%3").arg(c).arg(hourIndex1).arg(hourIndex2));
			m_currentData[c] = m_data[c][hourIndex1] * alpha + m_data[c][hourIndex2] * (1-alpha);
		}
		else {
			IBK_ASSERT_XX(m_overrideData[c].size() > std::max(hourIndex1, hourIndex2),
						  IBK::FormatString("#CC=%1, hourIndex1=%2, hourIndex2=%3").arg(c).arg(hourIndex1).arg(hourIndex2));
			m_currentData[c] = m_overrideData[c].value(t);
		}
	}
}


std::string ClimateDataLoader::composeErrorText(ReadFunctionReturnValues res, const IBK::Path & fullPath, const std::string & errorLine) {
	switch (res) {
		case RF_InvalidFilePath :
			return IBK::FormatString("Cannot open file '%1'.").arg(fullPath).str();
		case RF_BadHeader :
			return IBK::FormatString("Bad header format in line %1").arg(errorLine).str();
		case RF_BadLineFormat :
			return IBK::FormatString("Invalid format of line %1").arg(errorLine).str();
		case RF_InvalidTimeStamp :
			return IBK::FormatString("Invalid time point (non monotonic increasing or duplicate time point in line %1").arg(errorLine).str();
		default: ;
			return IBK::FormatString("Unknown error in line %1\n(CCM developers should add error text here!)").arg(errorLine).str();
	}
}


void ClimateDataLoader::checkForValidCyclicData(const std::vector<double> & timeVec) {
	const char * const FUNC_ID = "[ClimateDataLoader::checkForValidCyclicData]";
	if (timeVec.size() < 2)
		throw IBK::Exception("Missing data, at least two time points are required.", FUNC_ID);
	if (timeVec.back() > 365*24*3600)
		throw IBK::Exception("Time points must not exceed 365 d for cyclic use.", FUNC_ID);
	if (timeVec.front() == 0 && timeVec.back() == 365*24*3600)
		throw IBK::Exception("Must not have time point 0 d and 365 d for cyclic use.", FUNC_ID);
}


void ClimateDataLoader::expandToAnnualHourlyData(std::vector<double> & timeVec, std::vector<double> & dataVec) {
	const char * const FUNC_ID = "[ClimateDataLoader::expandToAnnualHourlyData]";
	// ensure consistent input data
	checkForValidCyclicData(timeVec);
	// populate temporary input vectors
	std::vector<double> tp, val;
	tp.swap(timeVec);
	val.swap(dataVec);
	timeVec.reserve(8760);
	dataVec.reserve(8760);

	// keep first time-value pair
	timeVec.push_back(tp.front());
	dataVec.push_back(val.front());

	// now process time points in vector
	for (unsigned int i=1; i<tp.size(); ++i) {
		double t = tp[i];
		unsigned int t_sec = (unsigned int)t;
		if (t != (double)t_sec || (t_sec % 3600 != 0))
			throw IBK::Exception( IBK::FormatString("Time point '%1' is not a valid hourly value in seconds.").arg(t, 0, 'g', 10), FUNC_ID);

		// check if current time point matches this time point
		double last_t = timeVec.back();
		if (t > last_t + 3600) {
			// check that last and next value are the same
			if (dataVec.back() != val[i])
				throw IBK::Exception( IBK::FormatString("Missing hourly value and previous value '%1' (at time point %2 s) and next value '%3' differ.")
					.arg(dataVec.back()).arg(last_t).arg(val[i]), FUNC_ID);
			// keep inserting the same value until we reached our current time point
			while (t > last_t + 3600) {
				last_t += 3600;
				timeVec.push_back(last_t);
				dataVec.push_back(dataVec.back());
			}
		}
		// we have a new time point, add time point and corresponding value
		timeVec.push_back(t);
		dataVec.push_back(val[i]);
	}

	if (timeVec.size() != 8760)
		throw IBK::Exception("Error in expansion algorithm, more than 8760 hourly values in data series.", FUNC_ID);

	// finally, move time point 0d to 365d
	if (timeVec.front() == 0) {
		tp.swap(timeVec);
		dataVec.swap(val);
		timeVec.clear();
		dataVec.clear();
		timeVec.reserve(8760);
		dataVec.reserve(8760);
		// copy all values in range [1;8759]
		for (unsigned int i=1; i<8760; ++i) {
			timeVec.push_back(tp[i]);
			dataVec.push_back(val[i]);
		}
		timeVec.push_back(365*24*3600); // insert last hour of year
		dataVec.push_back(val.front()); // insert value at 0d for 365 d
	}

}


double ClimateDataLoader::interpolateCyclicData(const IBK::LinearSpline & spl, double timeOfYearInSeconds) {
	IBK_ASSERT(spl.valid());
	double t1 = spl.x().front();
	double tn = spl.x().back();
	if (timeOfYearInSeconds < t1 || timeOfYearInSeconds > tn) {
		tn -=365*24*3600; // apply cyclic treatment
		IBK_ASSERT(t1 > tn);
		// now interpolate linearly in this interval
		double alpha = (timeOfYearInSeconds-tn)/(t1-tn);
		return spl.y().back()*(1-alpha) + spl.y().front()*alpha;
	}
	else
		return spl.value(timeOfYearInSeconds);
}

} // namespace CCM

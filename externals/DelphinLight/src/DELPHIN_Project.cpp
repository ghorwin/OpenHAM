/*	Copyright (c) 2001-2017, Institut für Bauklimatik, TU Dresden, Germany

	Written by A. Nicolai, H. Fechner, St. Vogelsang, A. Paepcke, J. Grunewald
	All rights reserved.

	This file is part of the DelphinLight Library.

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

#include "DELPHIN_Project.h"

#include <sstream>
#include <set>
#include <vector>

#include <IBK_Exception.h>
#include <IBK_FileUtils.h>
#include <IBK_algorithm.h>
#include <IBK_UnitVector.h>
#include <IBK_Parameter.h>
#include <IBK_rectangle.h>

#include <CCM_ClimateDataLoader.h>

#include "DELPHIN_MaterialReference.h"

#include <tinyxml.h>

namespace DELPHIN_LIGHT {

Project::Project() {
	m_initialT.set("DefaultTemperature", "20 C");
	m_initialRH.set("DefaultRelativeHumidity", "80 %");
	m_simStart.set("Start", "0 d");
	m_simDuration.set("Duration", "5 d");
	m_maxTimeStep.set("MaxTimeStep", "30 d");
	m_minTimeStep.set("MinTimeStep", "1e-8 s");
	m_initialTimeStep.set("InitialTimeStep", "0.01 s");
	m_relTol.set("RelTol", "1e-5 ---");
	m_absTol.set("AbsTolMoisture", "1e-6 kg");
	m_dtOutput.set("StepSize", 1, "h"); // by default, hourly outputs
}


void Project::readXML(const IBK::Path & fileNamePath) {
	const char * const FUNC_ID = "[Project::readXML]";

	std::string fname = fileNamePath.str();
	m_filename = fname;
	IBK::IBK_Message(IBK::FormatString("Reading project file '%1'.\n").arg(fileNamePath.filename()), IBK::MSG_PROGRESS, FUNC_ID, IBK::VL_STANDARD);
	IBK::MessageIndentor indent; (void)indent;

	// at first, a filename without path placeholders will be stored, but can later
	// changed to hold the original filename including placeholders.

	// try to extract an ID from the filename, defaults to 0 if format is invalid
	m_id = IBK::extract_ID_from_filename( IBK::Path(fname) );
	// open material file
	TiXmlDocument doc( fname.c_str() );
	if (!doc.LoadFile()) {
		throw IBK::Exception(IBK::FormatString("Error in line %1 of construction type file:\n%2")
				.arg(doc.ErrorRow()).arg(doc.ErrorDesc()), FUNC_ID);
	}

	// we use a handle so that NULL pointer checks are done during the query functions
	TiXmlHandle xmlHandleDoc(&doc);

	const TiXmlElement * xmlRootElement = xmlHandleDoc.FirstChildElement().Element();
	if (!xmlRootElement)
		return; // empty project, this means we are using only defaults

	if (xmlRootElement->ValueStr() != "DelphinProject")
		throw IBK::Exception("Expected DelphinProject as root node in XML file.", FUNC_ID);

	// now we parse the content
	TiXmlHandle xmlRoot = TiXmlHandle((TiXmlNode*)xmlRootElement);


	// *** Project Info ***

	const TiXmlElement * xmlElem = xmlRoot.FirstChild( "ProjectInfo" ).Element();
	if (xmlElem) {
		// project information are construction type information
		// loop over all elements in this XML element
		for (const TiXmlElement * e = xmlElem->FirstChildElement(); e; e = e->NextSiblingElement()) {
			// get element name
			std::string name = e->Value();
			// handle known elements
			if (name == "Comment") {
				m_comment = e->GetText();
			}
			else if (name == "DisplayName") {
				m_displayName = e->GetText();
			}
			else {
				IBK::IBK_Message(IBK::FormatString(
						"Unknown element '%1' in ProjectInfo section.").arg(name), IBK::MSG_WARNING);
			}
		}
	}


	// *** Directory Placeholders ***

	xmlElem = xmlRoot.FirstChild( "DirectoryPlaceholders" ).Element();
	if (xmlElem) {
		// loop over all elements in this XML element
		for (const TiXmlElement * e=xmlElem->FirstChildElement(); e; e = e->NextSiblingElement()) {
			// get element name
			std::string name = e->Value();
			// handle known elements
			if (name == "Placeholder") {
				// search for attribute with given name
				const TiXmlAttribute* attrib = TiXmlAttribute::attributeByName(e, "name");
				if (attrib == NULL) {
					IBK::IBK_Message(IBK::FormatString(
							"Missing '%1' attribute in Placeholder element.").arg("name"), IBK::MSG_WARNING);
					continue;
				}
				m_placeholders[attrib->Value()] = e->GetText();
			}
			else {
				IBK::IBK_Message(IBK::FormatString(
						"Unknown element '%1' in DirectoryPlaceholders section.").arg(name), IBK::MSG_WARNING);
			}
		}
	}

	// insert Project Directory placeholder
	IBK::Path projectDir = fileNamePath.parentPath();
	if (projectDir.str().empty())
		projectDir = IBK::Path("./");
	m_placeholders["Project Directory"] = projectDir;


	// *** Init ***

	const TiXmlNode * xmlNode = xmlRoot.FirstChild( "Init" ).ToNode();
	if (xmlNode) {
		IBK::IBK_Message("Reading Init section\n", IBK::MSG_PROGRESS, FUNC_ID, IBK::VL_STANDARD);
		IBK::MessageIndentor indent; (void)indent;
		// SimulationParameter
		const TiXmlNode * xmlNodeSub = xmlNode->FirstChild("SimulationParameter");
		if (xmlNodeSub) {
			const TiXmlNode * xmlNode2 = xmlNodeSub->FirstChild("Interval");
			if (xmlNode2) {
				findAndReadParameter(xmlNode2, "Start", m_simStart, true);
				findAndReadParameter(xmlNode2, "Duration", m_simDuration, true);
			}
			xmlNode2 = findTagByNameAttribute(xmlNodeSub, "DefaultInitialCondition", "DefaultTemperature", true);
			if (xmlNode2)
				readParameter(xmlNode2, m_initialT);
			xmlNode2 = findTagByNameAttribute(xmlNodeSub, "DefaultInitialCondition", "DefaultRelativeHumidity", true);
			if (xmlNode2)
				readParameter(xmlNode2, m_initialRH);
		}
		// SolverParameter
		xmlNodeSub = xmlNode->FirstChild("SolverParameter");
		if (xmlNodeSub) {
			findAndReadParameter(xmlNodeSub, "MaxTimeStep", m_maxTimeStep, true);
			findAndReadParameter(xmlNodeSub, "MinTimeStep", m_minTimeStep, true);
			findAndReadParameter(xmlNodeSub, "InitialTimeStep", m_initialTimeStep, true);
			findAndReadParameter(xmlNodeSub, "RelTol", m_relTol, true);
			findAndReadParameter(xmlNodeSub, "AbsTolMoisture", m_absTol, true);
		}
	}

	// *** Materials ***

	xmlElem = xmlRoot.FirstChild( "Materials" ).Element();
	try {
		if (xmlElem) {
			IBK::IBK_Message("Reading Material section\n", IBK::MSG_PROGRESS, FUNC_ID, IBK::VL_STANDARD);
			IBK::MessageIndentor indent; (void)indent;
			// for each entry, create an insance of MaterialReference and call readXML() member function
			// to parse the XML tag, than append the instance to vector matRefs
			IBK::read_range_XML(xmlElem->FirstChildElement(), m_matRefs);
		}
	}
	catch (IBK::Exception & ex) {
		throw IBK::Exception(ex, IBK::FormatString("Error reading Materials database section."), FUNC_ID);
	}


	// *** Discretization (Grid) ***

	xmlNode = xmlRoot.FirstChild( "Discretization" ).Node();
	if (xmlNode) {
		IBK::IBK_Message("Reading Discretization section\n", IBK::MSG_PROGRESS, FUNC_ID, IBK::VL_STANDARD);
		IBK::MessageIndentor indent; (void)indent;
		const TiXmlElement * e = xmlNode->FirstChildElement("XSteps");
		if (!e)
			throw IBK::Exception("Missing 'XSteps' element within discretization section.", FUNC_ID);
		std::string unit, values;
		try {
			TiXmlElement::readSingleAttributeElement(e, "unit", unit, values);
			IBK::UnitVector uvec;
			uvec.read(values + " " + unit);
			// create material layers
			m_materialLayers.resize(uvec.size() );
			for (unsigned int i=0; i<uvec.size(); ++i) {
				m_materialLayers[i].m_width.name = "width";
				m_materialLayers[i].m_width.value = uvec[i];
				m_materialLayers[i].m_width.IO_unit.set(unit);
			}
		}
		catch (IBK::Exception & ex) {
			throw IBK::Exception(ex, IBK::FormatString(
				"Invalid format of 'XSteps' element."), FUNC_ID);
		}
	}
	else {
		throw IBK::Exception("Missing 'Discretization' section.", FUNC_ID);

	}


	// *** Assignments ***

	xmlElem = xmlRoot.FirstChild( "Assignments" ).Element();
	if (xmlElem) {
		IBK::IBK_Message("Reading Assignments section\n", IBK::MSG_PROGRESS, FUNC_ID, IBK::VL_STANDARD);
		IBK::MessageIndentor indent; (void)indent;
		// loop over all elements in this XML element
		for (const TiXmlElement * e = xmlElem->FirstChildElement(); e; e = e->NextSiblingElement()) {
			// this must be an Assignment
			if (e->ValueStr() != "Assignment")
				throw IBK::Exception(IBK::FormatString("Expected 'Assignment' tag within 'Assignments' section."), FUNC_ID);
			// read assignment
			const TiXmlAttribute * attrib = TiXmlAttribute::attributeByName(e, "type");
			if (!attrib)
				throw IBK::Exception(IBK::FormatString("Expected 'type' attribute in Assignment."), FUNC_ID);
			std::string str = attrib->Value();
			if (str == "Interface") {
				attrib = TiXmlAttribute::attributeByName(e, "location");
				if (!attrib || (attrib->ValueStr() != "Left" && attrib->ValueStr() != "Right"))
					throw IBK::Exception(IBK::FormatString("Missing or invalid 'location' attribute in Interface assignment, expected 'Left' or Right."), FUNC_ID);
				if (attrib->ValueStr() == "Left") {
					if (!m_leftInterfaceName.empty())
						throw IBK::Exception(IBK::FormatString("Duplicate left-side Interface assignment."), FUNC_ID);
					// read Interface name
					for (const TiXmlElement * esub = e->FirstChildElement(); esub; esub = esub->NextSiblingElement()) {
						// determine data based on element name
						std::string ename = esub->Value();
						if (ename == "Reference") {
							m_leftInterfaceName = esub->GetText();
							IBK::IBK_Message(IBK::FormatString("Left side interface '%1' assigned\n").arg(m_leftInterfaceName), IBK::MSG_PROGRESS, FUNC_ID, IBK::VL_STANDARD);
							break;
						}
					}
					if (m_leftInterfaceName.empty())
						throw IBK::Exception(IBK::FormatString("Missing or empty Reference tag in Interface assignment."), FUNC_ID);
					continue;
				}
				else {
					if (!m_rightInterfaceName.empty())
						throw IBK::Exception(IBK::FormatString("Duplicate right-side Interface assignment."), FUNC_ID);
					// read Interface name
					for (const TiXmlElement * esub = e->FirstChildElement(); esub; esub = esub->NextSiblingElement()) {
						// determine data based on element name
						std::string ename = esub->Value();
						if (ename == "Reference") {
							m_rightInterfaceName = esub->GetText();
							IBK::IBK_Message(IBK::FormatString("Right side interface '%1' assigned\n").arg(m_rightInterfaceName), IBK::MSG_PROGRESS, FUNC_ID, IBK::VL_STANDARD);
							break;
						}
					}
					if (m_rightInterfaceName.empty())
						throw IBK::Exception(IBK::FormatString("Missing or empty Reference tag in Interface assignment."), FUNC_ID);
					continue;
				}

			}
			else if (str != "Material") {
				IBK::IBK_Message(IBK::FormatString("Assignments of type '%1' are ignored.").arg(str), IBK::MSG_WARNING, FUNC_ID, IBK::VL_STANDARD);
				continue;
			}
			attrib = TiXmlAttribute::attributeByName(e, "location");
			if (!attrib || attrib->ValueStr() != "Element")
				throw IBK::Exception(IBK::FormatString("Missing or invalid 'location' attribute in Material assignment, expected 'Element'."), FUNC_ID);

			// temporary variables
			IBK::rectangle<int> range(-1,-1,-1,-1);


			// read sub-elements
			std::string refName;
			for (const TiXmlElement * esub = e->FirstChildElement(); esub; esub = esub->NextSiblingElement()) {
				// determine data based on element name
				std::string ename = esub->Value();
				try {
					if (ename == "Reference") {
						refName = esub->GetText();
						IBK::IBK_Message(IBK::FormatString("Material '%1' assigned\n").arg(refName), IBK::MSG_PROGRESS, FUNC_ID, IBK::VL_STANDARD);
					}
					else if (ename == "Range") {
						std::stringstream strm(esub->GetText());
						if (!(strm >> range)) {
							throw IBK::Exception(IBK::FormatString("Invalid format of Range element in Assignment."), FUNC_ID);
						}
//						if (range.left != range.right) {
//							throw IBK::Exception(IBK::FormatString("Invalid format of Range element in Assignment: only single-layer assignments are allowed (left and right must hold the same value)."), FUNC_ID);
//						}
						if (range.top != 0 || range.bottom != 0) {
							throw IBK::Exception(IBK::FormatString("Invalid format of Range element in Assignment: top and bottom indexes must be zero."), FUNC_ID);
						}
					}
					else {
						throw IBK::Exception(IBK::FormatString(
							"Unknown XML tag with name '%1' in Assignment element.").arg(ename), FUNC_ID);
					}
				}
				catch (IBK::Exception & ex) {
					throw IBK::Exception(ex, IBK::FormatString("Error reading element '%1' in Assignment element.").arg(ename), FUNC_ID);
				}
				catch (std::exception & ex) {
					// also catch std-exceptions that may originate from the TiCPP lib
					throw IBK::Exception(IBK::FormatString("%1\nError reading element '%2 in Assignment element.").arg(ex.what()).arg(ename), FUNC_ID);
				}
			}

			// if we have both IdNr and range, we can complete the material layer data
			if (refName.empty() || range.left == -1)
				throw IBK::Exception(IBK::FormatString("Both 'Reference' and 'Range' tags are needed in Assignment element."), FUNC_ID);

			// more range checking
			if ((unsigned int)range.left >= m_materialLayers.size() || (unsigned int)range.right >= m_materialLayers.size())
				throw IBK::Exception(IBK::FormatString("Range element holds invalid element range (out of bounds)."), FUNC_ID);

			// look-up assigned material in material list
			for (unsigned int i=0; i<m_matRefs.size(); ++i) {
				if (m_matRefs[i].m_name == refName) {
					// found the material reference, copy over to material layers
					for (int r=range.left; r<=range.right; ++r) {
						m_materialLayers[r].m_matRef = &m_matRefs[i];
					}
					break;
				}
			}

			// WARNING: Now that pointers to material reference objects have been assigned to m_materialLayers,
			//			their memory location MUST NOT CHANGE! Therefore, do not touch the m_matRefs vector anymore!

			// check assigned material for correctness
			if (m_materialLayers[range.left].m_matRef == NULL) {
				throw IBK::Exception(IBK::FormatString("Material referenced by ID name '%1' is "
					"not listed in material references section.").arg(refName), FUNC_ID);
			}
		}

	} // Assignments


	// *** Conditions ***

	xmlElem = xmlRoot.FirstChild( "Conditions" ).Element();
	if (xmlElem) {
		const TiXmlElement * xmlElemInt = NULL;
		for (const TiXmlElement * esub = xmlElem->FirstChildElement(); esub; esub = esub->NextSiblingElement()) {
			// determine data based on element name
			std::string ename = esub->Value();
			if (ename == "Interfaces") {
				xmlElemInt = esub;
				break;
			}
		}
		if (!xmlElemInt)
			throw IBK::Exception("Missing 'Interfaces' section.", FUNC_ID);
		// loop over all elements in this XML element
		for (const TiXmlElement * e = xmlElemInt->FirstChildElement(); e; e = e->NextSiblingElement()) {
			std::string ename = e->Value();
			if (ename != "Interface")
				throw IBK::Exception( IBK::FormatString("Expected 'Interface' element, got '%1'").arg(ename), FUNC_ID);
			const TiXmlAttribute * attrib = TiXmlAttribute::attributeByName(e, "name");
			if (!attrib)
				throw IBK::Exception(IBK::FormatString("Missing 'name' attribute in Interface definition."), FUNC_ID);
			Interface * iface = NULL;
			if (attrib->ValueStr() == m_leftInterfaceName)
				iface = &m_leftInterface;
			else if (attrib->ValueStr() == m_rightInterfaceName)
				iface = &m_rightInterface;
			else {
				throw IBK::Exception( IBK::FormatString("Invalid/unassigned interface definition '%1' found.").arg(attrib->ValueStr()), FUNC_ID);
			}
			IBK::IBK_Message(IBK::FormatString("Parsing definition of Interface '%1'\n").arg(attrib->ValueStr()), IBK::MSG_PROGRESS, FUNC_ID, IBK::VL_STANDARD);
			IBK::MessageIndentor indent3; (void)indent3;

			attrib = TiXmlAttribute::attributeByName(e, "type");
			if (!attrib || attrib->ValueStr() != "Detailed")
				throw IBK::Exception(IBK::FormatString("Missing or invalid 'type' attribute in Interface definition, expected 'Detailed'."), FUNC_ID);
			// now parse interface structure
			for (const TiXmlElement * esub = e->FirstChildElement(); esub; esub = esub->NextSiblingElement()) {
				// determine data based on element name
				std::string ename = esub->Value();
				if (ename == "BCReference") {
					readBCPara(xmlElem, iface, esub->GetText());
				}
				else
					throw IBK::Exception(IBK::FormatString("Expected 'BCReference' tag, got '%1'.").arg(ename), FUNC_ID);
			}

		}
	}


	// *** Outputs ***
	xmlElem = xmlRoot.FirstChild( "Outputs" ).Element();
	if (xmlElem) {
		// get first output grid child element
		const TiXmlNode * xmlNodeSub = xmlElem->FirstChild("OutputGrids");
		if (xmlNodeSub) {
			xmlNodeSub = xmlNodeSub->FirstChildElement();
			if (xmlNodeSub) {
				xmlNodeSub = xmlNodeSub->FirstChild("Interval");
				if (xmlNodeSub) {
					for (const TiXmlElement * e = xmlNodeSub->FirstChildElement(); e; e = e->NextSiblingElement()) {
						double value;
						std::string unitStr, name;
						try {
							TiXmlElement::readIBKParameterElement(e, name, unitStr, value);
						}
						catch (std::runtime_error & ex) {
							throw IBK::Exception( IBK::Exception(ex.what(), "[TiXmlElement::readIBKParameterElement]"), "Error reading parameter from OutputGrid.", FUNC_ID);
						}
						if (name == "StepSize") {
							m_dtOutput.set(name, value, unitStr);
							break;
						}
					}
				}
			}
		}
	}
}


void Project::writeXML(const std::string & fname) const {
	const char * const FUNC_ID = "[Project::writeXML]";

	TiXmlDocument doc;
	TiXmlDeclaration * decl = new TiXmlDeclaration( "1.0", "UTF-8", "" );
	doc.LinkEndChild( decl );

	TiXmlElement * parent = new TiXmlElement( "DelphinProject" );
	doc.LinkEndChild(parent);

//	parent->SetAttribute("xmlns", "http://www.bauklimatik-dresden.de");
//	parent->SetAttribute("xmlns:IBK", "http://www.bauklimatik-dresden.de/IBK");
//	parent->SetAttribute("xsi:schemaLocation", "http://www.bauklimatik-dresden.de DelphinProject.xsd");
	parent->SetAttribute("fileVersion", "6.2.0");


	// write Project Information
	if (!m_comment.empty() || !m_displayName.empty()) {

		TiXmlElement * e1 = new TiXmlElement( "ProjectInfo" );
		parent->LinkEndChild( e1 );

		if (!m_comment.empty())
			TiXmlElement::appendSingleAttributeElement(e1, "Comment", NULL, std::string(), m_comment);
		if (!m_displayName.empty())
			TiXmlElement::appendSingleAttributeElement(e1, "DisplayName", NULL, std::string(), m_displayName);
	}

	// write placeholders
	if (!m_placeholders.empty()) {
		TiXmlElement * e1 = new TiXmlElement( "DirectoryPlaceholders" );
		parent->LinkEndChild( e1 );

		for (std::map<std::string,IBK::Path>::const_iterator it = m_placeholders.begin();
			 it != m_placeholders.end(); ++it)
		{
			TiXmlElement::appendSingleAttributeElement(	e1,
														"Placeholder",
														"name",
														it->first,
														it->second.str());
		}
	}

	// write materials
	// first we need to get a list of unique materials
	std::set<std::string> matIDs;
	for (unsigned int i=0; i<m_materialLayers.size(); ++i) {
		if (m_materialLayers[i].m_matRef != NULL)
			matIDs.insert(m_materialLayers[i].m_matRef->m_name);
		else
			throw IBK::Exception(IBK::FormatString("Missing material referenace assigned to layer #%1.").arg(i), FUNC_ID);
	}

	// remove potential empty Ids
	matIDs.erase(std::string());

	if (!matIDs.empty()) {
		TiXmlElement * e = new TiXmlElement("Materials");
		parent->LinkEndChild(e);
		// process all referenced materials
		for (std::set<std::string>::const_iterator it = matIDs.begin(); it != matIDs.end(); ++it) {
			// find a material layer that references this material ID
			for (unsigned int i=0; i<m_materialLayers.size(); ++i) {
				if (m_materialLayers[i].m_matRef->m_name == *it) {
					// write material reference
					m_materialLayers[i].m_matRef->writeXML(e);
					break;
				}
			}
		}
	}


	// write Discretization
	if (!m_materialLayers.empty()) {
		TiXmlElement * xmlDisc = new TiXmlElement( "Discretization" ); parent->LinkEndChild( xmlDisc );
		xmlDisc->SetAttribute("geometryType", "2D");

		IBK::UnitVector uvec;
		uvec.m_unit = m_materialLayers[0].m_width.IO_unit;
		for (unsigned int i=0; i<m_materialLayers.size(); ++i)
			uvec.m_data.push_back(m_materialLayers[i].m_width.value);
		std::string values = uvec.toString(false);
		TiXmlElement::appendSingleAttributeElement(xmlDisc, "XSteps",
			"unit", uvec.m_unit.name(), values);
	}

	// write Assignments
	if (!m_materialLayers.empty()) {
		TiXmlElement * xmlAssignments = new TiXmlElement( "Assignments" ); parent->LinkEndChild( xmlAssignments );
		for (unsigned int i=0; i<m_materialLayers.size(); ++i) {
			IBK::rectangle<unsigned int> range(i,0,i,0);

			TiXmlElement * e = new TiXmlElement("Assignment");
			xmlAssignments->LinkEndChild(e);

			// attributes
			e->SetAttribute("type", "Material");
			e->SetAttribute("location", "Element");
			// add sub-elements
			std::string idstr = IBK::val2string(m_materialLayers[i].m_matRef->m_name);
			// write Reference tag
			TiXmlElement::appendSingleAttributeElement(e, "Reference", NULL, std::string(), idstr);
			TiXmlElement::appendSingleAttributeElement(e, "Range", NULL, std::string(), range.toString());
		}

	}

	doc.SaveFile( fname.c_str() );
}



unsigned int Project::id() const {
	return m_id;
}


void Project::setId(unsigned int id) {
	m_id = id;
}


const std::string & Project::filename() const {
	return m_filename;
}


void Project::setFilename(const std::string & fname) {
	m_filename = fname;
}


const std::string & Project::displayName() const {
	return m_displayName;
}


void Project::setDisplayName(const std::string & dname) {
	m_displayName = dname;
}


bool Project::containsMaterialReference( const std::string & refName ) {

	for (	std::vector< MaterialLayer >::const_iterator it = m_materialLayers.begin(),
			end = m_materialLayers.end();
			it != end;
			++it
		)
	{
		if ( (*it).m_matRef != NULL && (*it).m_matRef->m_name == refName ){
			return true;
		}
	}

	return false;

}

void Project::materialFiles(std::set<IBK::Path> & filePaths) const {
	// loop through all material layers
	for (unsigned int i=0; i<m_materialLayers.size(); ++i) {
		// get relative file name and substitute path placeholders

		if (m_materialLayers[i].m_matRef == NULL)
			continue;
		IBK::Path myPath(m_materialLayers[i].m_matRef->m_filename);
		filePaths.insert( myPath.withReplacedPlaceholders( m_placeholders ) );
	}
}


void Project::readBCPara(const TiXmlElement * conditionsXmlElem, Interface * iface, const std::string & bcDefinition) {
	const char * const FUNC_ID = "[Project::readBCPara]";

	// find correct parameter block
	const TiXmlNode * bc = findConditionTag(conditionsXmlElem, "BoundaryConditions", "BoundaryCondition", bcDefinition);
	// now read BC parameters
	IBK::IBK_Message(IBK::FormatString("Parsing BC parameter definition '%1'\n").arg(bcDefinition), IBK::MSG_PROGRESS, FUNC_ID, IBK::VL_STANDARD);
	IBK::MessageIndentor indent; (void)indent;

	const TiXmlElement * bcElement = bc->ToElement();
	const TiXmlAttribute * attrib = TiXmlAttribute::attributeByName(bcElement, "type");
	if (!attrib)
		throw IBK::Exception(IBK::FormatString("Missing 'type' attribute in condition definition."), FUNC_ID);
	const TiXmlAttribute * attribKind = TiXmlAttribute::attributeByName(bcElement, "kind");
	if (!attribKind)
		throw IBK::Exception(IBK::FormatString("Missing 'kind' attribute in condition definition."), FUNC_ID);

	// heat conduction parameter
	if (attrib->ValueStr() == "HeatConduction") {
		// currently we only support kind 'Exchange'
		if (attribKind->ValueStr() != "Exchange")
			throw IBK::Exception(IBK::FormatString("Currently only 'Exchange' model kind is supported for 'HeatConduction' BC."), FUNC_ID);
		// we need to parse the exchange coefficient child and the CCReference of type 'Temperature'
		try {
			findAndReadParameter(bc, "ExchangeCoefficient", iface->alpha);
			readCCData(conditionsXmlElem, bc, "Temperature", iface->T, iface->T_spline);
		}
		catch (IBK::Exception & ex) {
			throw IBK::Exception(ex, IBK::FormatString("Error parsing BC parameter block with name '%1'").arg(bcDefinition), FUNC_ID);
		}

	}
	// vapor diffusion
	else if (attrib->ValueStr() == "VaporDiffusion") {
		// currently we only support kind 'Exchange'
		if (attribKind->ValueStr() != "Exchange")
			throw IBK::Exception(IBK::FormatString("Currently only 'Exchange' model kind is supported for 'VaporDiffusion' BC."), FUNC_ID);
		// we need to parse the exchange coefficient child and the CCReference of type 'Temperature' and 'RelativeHumidity'
		try {
			findAndReadParameter(bc, "ExchangeCoefficient", iface->beta);
			readCCData(conditionsXmlElem, bc, "Temperature", iface->T, iface->T_spline);
			readCCData(conditionsXmlElem, bc, "RelativeHumidity", iface->RH, iface->RH_spline);
		}
		catch (IBK::Exception & ex) {
			throw IBK::Exception(ex, IBK::FormatString("Error parsing BC parameter block with name '%1'").arg(bcDefinition), FUNC_ID);
		}
	}
	// rain via water contact
	else if (attrib->ValueStr() == "WaterContact") {
		// currently we only support kind 'ImposedFlux'
		if (attribKind->ValueStr() != "ImposedFlux")
			throw IBK::Exception(IBK::FormatString("Currently only 'ImposedFlux' model kind is supported for 'WaterContact' BC."), FUNC_ID);
		// we need to parse the CCReference of type 'Temperature' and 'WaterFlux'
		try {
			readCCData(conditionsXmlElem, bc, "Temperature", iface->T, iface->T_spline);
			readCCData(conditionsXmlElem, bc, "WaterFlux", iface->rain, iface->rain_spline);
		}
		catch (IBK::Exception & ex) {
			throw IBK::Exception(ex, IBK::FormatString("Error parsing BC parameter block with name '%1'").arg(bcDefinition), FUNC_ID);
		}
	}



}




const TiXmlNode * Project::findTagByNameAttribute(const TiXmlNode * parent,
												  const std::string & tag,
												  const std::string & name,
												  bool optional)
{
	const char * const FUNC_ID = "[Project::findTagByNameAttribute]";
	// find correct parameter block
	for (const TiXmlElement * e = parent->FirstChildElement(); e; e = e->NextSiblingElement()) {
		std::string ename = e->ValueStr();
		if (ename == tag) {
			// search for name attribute
			const TiXmlAttribute * attrib = TiXmlAttribute::attributeByName(e, "name");
			if (!attrib)
				throw IBK::Exception(IBK::FormatString("Missing 'name' attribute in definition of '%1' tag.").arg(tag), FUNC_ID);
			if (attrib->ValueStr() == name)
				return (const TiXmlNode *)e;
		}
	}
	if (optional)
		return NULL;
	throw IBK::Exception( IBK::FormatString("Missing '%1' tag with attribute name '%2'")
						  .arg(tag).arg(name), FUNC_ID);
}


const TiXmlNode * Project::findConditionTag(const TiXmlNode * conditionsXmlElem,
													  const std::string & conditionType,
													  const std::string & conditionChildType,
													  const std::string & name)
{
	const char * const FUNC_ID = "[Project::findConditionTag]";

	// find correct parameter block
	const TiXmlElement * bcs = NULL;
	for (const TiXmlElement * e = conditionsXmlElem->FirstChildElement(); e; e = e->NextSiblingElement()) {
		std::string ename = e->ValueStr();
		if (ename == conditionType) {
			bcs = e;
			break;
		}
	}
	if (bcs == NULL)
		throw IBK::Exception( IBK::FormatString("Missing '%1' tag within Conditions").arg(conditionType), FUNC_ID);

	// search all BC parameters
	const TiXmlElement * bc = NULL;
	for (const TiXmlElement * e = bcs->FirstChildElement(); e; e = e->NextSiblingElement()) {
		std::string ename = e->ValueStr();
		if (ename != conditionChildType)
			throw IBK::Exception( IBK::FormatString("Expected '%1' tag within '%2'").arg(conditionChildType).arg(conditionType), FUNC_ID);

		const TiXmlAttribute * attrib = TiXmlAttribute::attributeByName(e, "name");
		if (!attrib)
			throw IBK::Exception(IBK::FormatString("Missing 'name' attribute in condition definition."), FUNC_ID);
		if (attrib->ValueStr() == name) {
			bc = e;
			break;
		}
	}
	if (bc == NULL)
		throw IBK::Exception( IBK::FormatString("Missing '%1' tag with name '%2'").arg(conditionChildType).arg(name), FUNC_ID);
	return bc;
}


void Project::findAndReadParameter(const TiXmlNode * parent,
								   const std::string & paraName,
								   IBK::Parameter & para,
								   bool optional) const
{
	const char * const FUNC_ID = "[Project::readParameter]";

	try {
		// if not optional, an exception is thrown if the parameter isn't found
		const TiXmlNode * paraNode = findTagByNameAttribute(parent, "IBK:Parameter", paraName, optional);
		if (!paraNode && optional)
			return;

		// parameter exists, only when parameter tag is malformed, an exception is thrown here
		readParameter(paraNode, para);
	}
	catch (IBK::Exception & ex) {
		throw IBK::Exception(ex, IBK::FormatString("Error reading IBK:Parameter tag with name '%1'.").arg(paraName), FUNC_ID);
	}
}


void Project::readParameter(const TiXmlNode * paraNode, IBK::Parameter & para) const {
	const char * const FUNC_ID = "[Project::readParameter]";

	const TiXmlElement * e = paraNode->ToElement();
	std::string name, unit;
	double val;
	TiXmlElement::readIBKParameterElement(e, name, unit, val);
	if (!para.set(name, val, unit))
		throw IBK::Exception( IBK::FormatString("Error parsing parameter tag with name '%1'").arg(name), FUNC_ID);
	IBK::IBK_Message(IBK::FormatString("%1 = %2 %3\n").arg(name).arg(val).arg(unit), IBK::MSG_PROGRESS, FUNC_ID, IBK::VL_STANDARD);
}


void Project::readCCData(const TiXmlNode * conditionsXmlElem, const TiXmlNode* bcXmlElem,
						 const std::string & ccName,
						 IBK::Parameter & para, IBK::LinearSpline & spline) const
{
	const char * const FUNC_ID = "[Project::readCCData]";

	try {
		const TiXmlNode * paraNode = NULL;
		// find correct parameter block
		for (const TiXmlElement * e = bcXmlElem->FirstChildElement(); e; e = e->NextSiblingElement()) {
			std::string ename = e->ValueStr();
			if (ename == "CCReference") {
				// search for name attribute
				const TiXmlAttribute * attrib = TiXmlAttribute::attributeByName(e, "type");
				if (!attrib)
					throw IBK::Exception(IBK::FormatString("Missing 'type' attribute in definition of 'CCReference' tag."), FUNC_ID);
				if (attrib->ValueStr() == ccName) {
					paraNode = (const TiXmlNode *)e;
					break;
				}
			}
		}
		if (!paraNode)
			throw IBK::Exception(IBK::FormatString("Missing 'CCReference' tag with name '%1'.").arg(ccName), FUNC_ID);


		const TiXmlElement * e = paraNode->ToElement();
		std::string ccRef = e->GetText();
		IBK::IBK_Message(IBK::FormatString("Reading CC Reference: [%1] = '%2'\n").arg(ccName).arg(ccRef), IBK::MSG_PROGRESS, FUNC_ID, IBK::VL_STANDARD);

		// can be a constant value in parameter format or a reference to a climate condition
		IBK::Parameter p;
		if (p.set("Temperature", ccRef)) {
			para = p;
			spline.clear();
			return;
		}

		// must be a ClimateCond
		const TiXmlNode * ccNode = findConditionTag(conditionsXmlElem, "ClimateConditions", "ClimateCondition", ccRef);
		if (!ccNode)
			throw IBK::Exception(IBK::FormatString("Missing ClimateCondition definition with ID name '%1'.").arg(ccRef), FUNC_ID);

		// get condition kind
		const TiXmlAttribute * attrib = TiXmlAttribute::attributeByName(ccNode->ToElement(), "kind");
		if (!attrib)
			throw IBK::Exception(IBK::FormatString("Missing 'kind' attribute in definition of 'ClimateCondition' tag."), FUNC_ID);
		if (attrib->ValueStr() == "Constant") {
			findAndReadParameter(ccNode, "ConstantValue", para);
			spline.clear();
		}
		else if (attrib->ValueStr() == "TabulatedData") {
			// must be a CCD or tsv file
			for (const TiXmlElement * e = ccNode->FirstChildElement(); e; e = e->NextSiblingElement()) {
				std::string ename = e->ValueStr();
				// read filename
				if (ename == "Filename") {
					std::string fname;
					std::string attribDummy;
					TiXmlElement::readSingleAttributeElement(e, NULL, attribDummy, fname);
					// subtitute placeholders
					IBK::Path fullpath = IBK::Path(fname).withReplacedPlaceholders( m_placeholders );
					// read CCD file
					CCM::ClimateDataLoader loader;
					std::vector<double> tp;
					IBK::UnitVector uvec;
					std::string comment, quantity, errorLine;
					IBK::Unit dataUnit;
					if (fullpath.extension() == "ccd") {
						if (loader.readCCDFile(fullpath, tp, uvec.m_data, comment, quantity, dataUnit, errorLine) != CCM::ClimateDataLoader::RF_OK) {
							throw IBK::Exception(IBK::FormatString("Error reading climate data file '%1', error in line '%2'")
												 .arg(fullpath).arg(errorLine), FUNC_ID);
						}
					}
					else if (fullpath.extension() == "csv" || fullpath.extension() == "tsv") {
						if (loader.readCSVFile(fullpath, tp, uvec.m_data, quantity, dataUnit, errorLine) != CCM::ClimateDataLoader::RF_OK) {
							throw IBK::Exception(IBK::FormatString("Error reading climate data file '%1', error in line '%2'")
												 .arg(fullpath).arg(errorLine), FUNC_ID);
						}
					}
					else
						throw IBK::Exception(IBK::FormatString("Unknown/unsupported file type for climate data file '%1', "
															   "supported are ccd, csv, tsv.").arg(fullpath), FUNC_ID);
					// create a spline
					para.clear();

					// convert units to base unit
					uvec.m_unit = dataUnit;
					uvec.convert( IBK::Unit(dataUnit.base_id()) );
					spline.setValues(tp, uvec.m_data);
					if (!spline.makeSpline(errorLine)) {
						throw IBK::Exception(IBK::FormatString("Error reading climate data file '%1', invalid spline data: '%2'")
											 .arg(fname).arg(errorLine), FUNC_ID);
					}
					break;
				}
			}
			if (spline.empty())
				throw IBK::Exception("Missing 'Filename' xml tag in 'ClimateCondition' tag of type 'TabulatedData'.", FUNC_ID);
		}
		else
			throw IBK::Exception(IBK::FormatString("Unsupported 'kind' for ClimateCondition, currently only 'Constant' supported for now."), FUNC_ID);

	}
	catch (IBK::Exception & ex) {
		throw IBK::Exception(ex, IBK::FormatString("Error reading CCReference '%1' in BC parameter block.").arg(ccName), FUNC_ID);
	}
}

} // namespace DELPHIN_LIGHT


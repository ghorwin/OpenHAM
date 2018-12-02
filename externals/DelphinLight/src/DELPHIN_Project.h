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

#ifndef DELPHIN_ProjectH
#define DELPHIN_ProjectH

#include <string>
#include <vector>
#include <map>

#include <IBK_Parameter.h>
#include <IBK_Path.h>
#include <IBK_LinearSpline.h>

#include "DELPHIN_MaterialReference.h"

class TiXmlElement;
class TiXmlNode;


/*! The namespace DELPHIN_LIGHT wraps the data types of the DelphinLight library. */
namespace DELPHIN_LIGHT {

/*!	The class Project represents actually a 1D construction type.
	The class supports serialization to a simplified version of a DELPHIN Project
	file with only X direction grids cells (no discretization), material references
	and material assignments.

	Upon reading an DELPHIN Project file with the readXML() function, the
	data is interpreted, checked for validity, and stored in the member variables for
	easy access and manipulation of 1D data

	\note Writing project data is not supported. writeXML() only writes a small fraction of
	data needed for simulation.
*/
class Project {
public:
	/*! This type wraps the data of a material reference and a layer width. */
	struct MaterialLayer {
		/*! The width of the material layer. */
		IBK::Parameter		m_width;
		/*! Pointer to the referenced material (not owned). */
		const MaterialReference	*m_matRef;

		/*! Initializing constructor. */
		MaterialLayer(double width, const MaterialReference & matref) :
			m_width("width", width, IBK::Unit("m")),
			m_matRef(&matref)
		{}

		/*! Default constructor. */
		MaterialLayer() :
			m_width("width", 0.1, IBK::Unit("m")), m_matRef(NULL)
		{}
	};

	/*! Holds interface parameter, boundary condition properties and climate condition data.
		Interface data structure is instantiated twice, left and right side of construction, see
		m_leftInterface and m_rightInterface.
	*/
	struct Interface {
		IBK::Parameter alpha;		///< Heat transfer exchange coefficient, base unit [W/m2K]
		IBK::Parameter beta;		///< Vapor diffusion exchange coefficient, base unit [s/m]
		IBK::Parameter T;			///< Temperature, base unit [K]
		IBK::Parameter RH;			///< Relative humiditiy, base unit [---]
		IBK::Parameter pv;			///< Vapor pressure, base unit [Pa]
		IBK::Parameter rain;		///< Water flux density, base unit [kg/m2s]
		IBK::LinearSpline T_spline;
		IBK::LinearSpline RH_spline;
		IBK::LinearSpline pv_spline;
		IBK::LinearSpline rain_spline;
		IBK::LinearSpline qswrad_spline;
	};

	// *** PUBLIC MEMBER FUNCTIONS ***

	/*! Default constructor, initializes parameters with defaults. */
	Project();

	/*! Reads from XML file in DELPHIN format.
		This function does a lot of error checking and ensures that the resulting
		content of the Project object is sane and valid for computation.
		The material references from the DELPHIN project file are kept including
		potential place holders. When the corresponding material files are to be read
		later on, the place holders must be substituted accordingly, using the
		placeholders stored in m_placeholders and optionally any predefined placeholders.
	*/
	void readXML(const IBK::Path & fileNamePath);
	/*! Writes to XML file in DELPHIN format.
		This function creates the content of a DELPHIN file (material list, discretization
		data and assignment list) and writes a fully conforming DELPHIN file (yet without
		assigned boundary conditions or outputs).
	*/
	void writeXML(const std::string & fname) const;
	/*! Returns unique ID. */
	unsigned int id() const;
	/*! Sets unique ID. */
	void setId(unsigned int id);
	/*! Returns current filename (may include placeholders). */
	const std::string & filename() const;
	/*! Sets current filename (may include placeholders). */
	void setFilename(const std::string & fname);
	/*! Returns current displayName. */
	const std::string & displayName() const;
	/*! Sets current displayName. */
	void setDisplayName(const std::string & dname);
	/*! Returns true if construction contains the referenced material id number. */
	bool containsMaterialReference( const std::string & refName );

	/*! Generates a set with material file paths (absolute file path, with placeholders
		already substituted).
		\param filePaths A set with absolute file paths of referenced material files. The set is
						 not cleared in this function, so it can be used in a loop over several
						 Projects to get a list of all referenced material files.
	*/
	void materialFiles(std::set<IBK::Path> & filePaths) const;


	/*! Reads a BC parameterization and adds the data to the corresponding interface data structure. */
	void readBCPara(const TiXmlElement * conditionsXmlElem, Interface * iface, const std::string & bcDefinition);

	// *** PUBLIC MEMBER VARIABLES ***

	/*! Id number of the construction type. */
	unsigned int					m_id;
	/*! Name of the input file (may include placeholders). */
	std::string						m_filename;
	/*! Some comment about the construction type, IBK-language encoded. */
	std::string						m_comment;
	/*! Name to be displayed as identifying name for this construction type, IBK-language encoded. */
	std::string						m_displayName;
	/*! List of directory placeholders. */
	std::map<std::string, IBK::Path> m_placeholders;

	/*! Material references.
		\note MaterialLayers hold pointers to MaterialReference objects. Once these pointers
			  are assigned in readXML(), DO NOT MODIFY the m_matRefs vector anylonger (otherwise SEGFAULTS/ACCESS VIOLOATIONS)
	*/
	std::vector<MaterialReference>	m_matRefs;

	/*! Material layers of the construction. */
	std::vector<MaterialLayer>		m_materialLayers;

	std::string						m_leftInterfaceName;
	std::string						m_rightInterfaceName;

	Interface						m_leftInterface;
	Interface						m_rightInterface;

	/*! Initial temperature. */
	IBK::Parameter					m_initialT;
	/*! Initial relative humidity. */
	IBK::Parameter					m_initialRH;

	// Simulation parameters read from project file
	// Mind: When reading DELPHIN 6 project files, parameters are only stored in the project file
	//       if different from the default conditions.
	//       Therefore, the defaults set in the construction of Project should be synchronized to those
	//       in the DELPHIN Project.
	IBK::Parameter					m_simStart;
	IBK::Parameter					m_simDuration;
	IBK::Parameter					m_maxTimeStep;
	IBK::Parameter					m_minTimeStep;
	IBK::Parameter					m_initialTimeStep;
	IBK::Parameter					m_relTol;
	IBK::Parameter					m_absTol;

	IBK::Parameter					m_dtOutput;

	// *** STATIC FUNCTIONS ***

	/*! Searches for an XML tag with a name attribute and given name.
		If optional parameter is false, an exception is thrown if such an XML tag is not found.
	*/
	static const TiXmlNode * findTagByNameAttribute(const TiXmlNode * parent,
													const std::string & tag,
													const std::string & name,
													bool optional = false);

	/*! Utility function that searches for a certain parameter block within the Conditions tag.
		\note Tag must exist, otherwise an exception is thrown.
	*/
	static const TiXmlNode * findConditionTag(const TiXmlNode * conditionsXmlElem,
											  const std::string & conditionType,
											  const std::string & conditionChildType,
											  const std::string & name);

	/*! Utility function that first searches for an IBK:Parameter child element by name and then reads it.
		\note If the 'optional' argument is not true, the parameter must exist and and exception is throw when
			  the parameter is missing. 'para' is only modified when the parameter could be read correctly.

		When the IBK:Parameter tag is incomplete, an exception is thrown.
	*/
	void findAndReadParameter(const TiXmlNode* parent,
							  const std::string & paraName,
							  IBK::Parameter & para,
							  bool optional = false) const;

	/*! Utility function that reads an IBK:Parameter by name from a given IBK:Parameter xml node.
		\note Paramter must exist and be valid, otherwise an exception is thrown.
	*/
	void readParameter(const TiXmlNode * paraNode, IBK::Parameter & para) const;

	/*! Utility function that reads a CCReference entry and its associated CC definition.
		\note Paramter must exist and be valid, otherwise an exception is thrown.
	*/
	void readCCData(const TiXmlNode * conditionsXmlElem,
					const TiXmlNode * bcXmlElem,
					const std::string & ccName,
					IBK::Parameter & para, IBK::LinearSpline & spline) const;

};

} // namespace DELPHIN_LIGHT

#endif // DELPHIN_ProjectH

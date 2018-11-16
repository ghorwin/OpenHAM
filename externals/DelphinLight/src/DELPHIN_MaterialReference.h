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

#ifndef DELPHIN_MaterialReferenceH
#define DELPHIN_MaterialReferenceH

#include <string>

#include <IBK_Path.h>
#include <IBK_Color.h>

class TiXmlElement;

namespace DELPHIN_LIGHT {

/*! A class that holds a material references (filename, ID and display name). */
class MaterialReference {
public:

	/*! Constructor. */
	MaterialReference();

	/*! Reads the data from the xml element.
		Throws an IBK::Exception if a syntax error occurs.
	*/
	void readXML(const TiXmlElement * element);

	/*! Appends the element to the parent xml element.
		Throws an IBK::Exception in case of invalid data.
	*/
	void writeXML(TiXmlElement * parent) const;

	/*! ID-name of referenced material. */
	std::string		m_name;
	/*! File path (potentially including placeholders). */
	IBK::Path		m_filename;
	/*! Color to be used for drawing this material (UNDEFINED_COLOR used as default). */
	IBK::Color		m_color;
	/*! Hatching code, 0 - no hatching, for drawing material. */
	unsigned int	m_hatchCode;
};

/*! Comparison operator== to find a material reference by name only. */
inline bool operator==(const MaterialReference& lhs, const std::string& rhs)  { return lhs.m_name == rhs; }
/*! Comparison operator!= to find a material reference by name only. */
inline bool operator!=(const MaterialReference& lhs, const std::string& rhs)  { return lhs.m_name != rhs; }

} // namespace DELPHIN_LIGHT

#endif // DELPHIN_MaterialReferenceH

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

#include "DELPHIN_MaterialReference.h"

#include <IBK_Exception.h>
#include <IBK_FileUtils.h>
#include <IBK_StringUtils.h>

#include <tinyxml.h>

using namespace std;

namespace DELPHIN_LIGHT {

const unsigned int UNDEFINED_COLOR = 0x01010101;

MaterialReference::MaterialReference() :
	m_color(UNDEFINED_COLOR),
	m_hatchCode(0)
{
}

void MaterialReference::readXML(const TiXmlElement * element) {
	const char * const FUNC_ID = "[MaterialReference::readXML]";
	const TiXmlAttribute* attrib = TiXmlAttribute::attributeByName(element, "name");
	if (attrib != NULL) {
		m_name = attrib->ValueStr();
	}
	else {
		m_name.clear();
	}

	attrib = TiXmlAttribute::attributeByName(element, "color");
	if (attrib != NULL) {
		std::string htmlColor = attrib->ValueStr();
		try {
			m_color = IBK::Color::fromHtml( htmlColor );
		}
		catch (IBK::Exception & ex) {
			throw IBK::Exception(ex, IBK::FormatString("Error decoding color from string '%1'.").arg(htmlColor), FUNC_ID);
		}
	}

	attrib = TiXmlAttribute::attributeByName(element, "hatchCode");
	if (attrib != NULL) {
		std::string hatchCode = attrib->ValueStr();
		try {
			m_hatchCode = IBK::string2val<unsigned int>(hatchCode);
		}
		catch (IBK::Exception &) {
			throw IBK::Exception(IBK::FormatString("Error decoding hatch code from string '%1'.").arg(hatchCode), FUNC_ID);
		}
	}
	const char * const str = element->GetText();
	if (str)
		m_filename = IBK::Path(str);
	else
		m_filename.clear();

}


void MaterialReference::writeXML(TiXmlElement * parent) const {
	if (!m_filename.isValid())
		return; // do not write invalid IDs
	TiXmlElement * xmlElement = new TiXmlElement( "MaterialReference" );
	parent->LinkEndChild( xmlElement );

	// set displayName
	xmlElement->SetAttribute("name", m_name);
	if (m_color != 0)
		xmlElement->SetAttribute("color", m_color.toHtmlString());
	if (m_hatchCode != 0)
		xmlElement->SetAttribute("hatchCode", IBK::val2string(m_hatchCode));

	// add value if any
	TiXmlText * text = new TiXmlText( m_filename.str() );
	xmlElement->LinkEndChild( text );
}


} // namespace DELPHIN_LIGHT


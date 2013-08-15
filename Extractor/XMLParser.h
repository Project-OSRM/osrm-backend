/*
 open source routing machine
 Copyright (C) Dennis Luxen, others 2010

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU AFFERO General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU Affero General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 or see http://www.gnu.org/licenses/agpl.txt.
 */

#ifndef XMLPARSER_H_
#define XMLPARSER_H_

#include "BaseParser.h"
#include "../DataStructures/Coordinate.h"
#include "../Util/SimpleLogger.h"
#include "../Util/StringUtil.h"
#include "../typedefs.h"

#include <libxml/xmlreader.h>


class XMLParser : public BaseParser {
public:
    XMLParser(const char* filename, ExtractorCallbacks* ec, ScriptingEnvironment& se);
    bool ReadHeader();
    bool Parse();

private:
    _RawRestrictionContainer _ReadXMLRestriction();
    ExtractionWay _ReadXMLWay();
    ImportNode _ReadXMLNode();
    xmlTextReaderPtr inputReader;
};

#endif /* XMLPARSER_H_ */

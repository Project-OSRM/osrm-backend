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

#include <libxml/xmlreader.h>

#include "../typedefs.h"
#include "BaseParser.h"
#include "ExtractorCallbacks.h"
#include "ScriptingEnvironment.h"

class XMLParser : public BaseParser<ExtractorCallbacks, _Node, _RawRestrictionContainer, _Way> {
public:
    XMLParser(const char * filename);
    virtual ~XMLParser();
    void RegisterCallbacks(ExtractorCallbacks * em);
    void RegisterScriptingEnvironment(ScriptingEnvironment & _se);
    bool Init();
    bool Parse();
    
private:
    _RawRestrictionContainer _ReadXMLRestriction();
    _Way _ReadXMLWay();
    ImportNode _ReadXMLNode( );
    /* Input Reader */
    xmlTextReaderPtr inputReader;
    ExtractorCallbacks * externalMemory;
    lua_State *myLuaState;

    std::vector<std::string> restriction_exceptions_vector;
};

#endif /* XMLPARSER_H_ */

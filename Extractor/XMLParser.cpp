/*

Copyright (c) 2013, Project OSRM, Dennis Luxen, others
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

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

#include "XMLParser.h"

#include "ExtractionWay.h"
#include "ExtractorCallbacks.h"

#include "../DataStructures/HashTable.h"
#include "../DataStructures/ImportNode.h"
#include "../DataStructures/InputReaderFactory.h"
#include "../DataStructures/Restriction.h"
#include "../Util/SimpleLogger.h"
#include "../Util/StringUtil.h"
#include "../typedefs.h"

#include <osrm/Coordinate.h>

XMLParser::XMLParser(const char *filename, ExtractorCallbacks *ec, ScriptingEnvironment &se)
    : BaseParser(ec, se)
{
    inputReader = inputReaderFactory(filename);
}

bool XMLParser::ReadHeader() { return (xmlTextReaderRead(inputReader) == 1); }
bool XMLParser::Parse()
{
    while (xmlTextReaderRead(inputReader) == 1)
    {
        const int type = xmlTextReaderNodeType(inputReader);

        // 1 is Element
        if (type != 1)
        {
            continue;
        }

        xmlChar *currentName = xmlTextReaderName(inputReader);
        if (currentName == NULL)
        {
            continue;
        }

        if (xmlStrEqual(currentName, (const xmlChar *)"node") == 1)
        {
            ImportNode n = ReadXMLNode();
            ParseNodeInLua(n, lua_state);
            extractor_callbacks->ProcessNode(n);
        }

        if (xmlStrEqual(currentName, (const xmlChar *)"way") == 1)
        {
            ExtractionWay way = ReadXMLWay();
            ParseWayInLua(way, lua_state);
            extractor_callbacks->ProcessWay(way);
        }
        if (use_turn_restrictions && xmlStrEqual(currentName, (const xmlChar *)"relation") == 1)
        {
            InputRestrictionContainer r = ReadXMLRestriction();
            if ((UINT_MAX != r.fromWay) && !extractor_callbacks->ProcessRestriction(r))
            {
                std::cerr << "[XMLParser] restriction not parsed" << std::endl;
            }
        }
        xmlFree(currentName);
    }
    return true;
}

InputRestrictionContainer XMLParser::ReadXMLRestriction()
{
    InputRestrictionContainer restriction;
    std::string except_tag_string;

    if (xmlTextReaderIsEmptyElement(inputReader) != 1)
    {
        const int depth = xmlTextReaderDepth(inputReader);
        while (xmlTextReaderRead(inputReader) == 1)
        {
            const int child_type = xmlTextReaderNodeType(inputReader);
            if (child_type != 1 && child_type != 15)
            {
                continue;
            }
            const int childDepth = xmlTextReaderDepth(inputReader);
            xmlChar *childName = xmlTextReaderName(inputReader);
            if (childName == NULL)
            {
                continue;
            }
            if (depth == childDepth && child_type == 15 &&
                xmlStrEqual(childName, (const xmlChar *)"relation") == 1)
            {
                xmlFree(childName);
                break;
            }
            if (child_type != 1)
            {
                xmlFree(childName);
                continue;
            }

            if (xmlStrEqual(childName, (const xmlChar *)"tag") == 1)
            {
                xmlChar *k = xmlTextReaderGetAttribute(inputReader, (const xmlChar *)"k");
                xmlChar *value = xmlTextReaderGetAttribute(inputReader, (const xmlChar *)"v");
                if (k != NULL && value != NULL)
                {
                    if (xmlStrEqual(k, (const xmlChar *)"restriction") &&
                        (0 == std::string((const char *)value).find("only_")))
                    {
                        restriction.restriction.flags.isOnly = true;
                    }
                    if (xmlStrEqual(k, (const xmlChar *)"except"))
                    {
                        except_tag_string = (const char *)value;
                    }
                }

                if (k != NULL)
                {
                    xmlFree(k);
                }
                if (value != NULL)
                {
                    xmlFree(value);
                }
            }
            else if (xmlStrEqual(childName, (const xmlChar *)"member") == 1)
            {
                xmlChar *ref = xmlTextReaderGetAttribute(inputReader, (const xmlChar *)"ref");
                if (ref != NULL)
                {
                    xmlChar *role = xmlTextReaderGetAttribute(inputReader, (const xmlChar *)"role");
                    xmlChar *type = xmlTextReaderGetAttribute(inputReader, (const xmlChar *)"type");

                    if (xmlStrEqual(role, (const xmlChar *)"to") &&
                        xmlStrEqual(type, (const xmlChar *)"way"))
                    {
                        restriction.toWay = stringToUint((const char *)ref);
                    }
                    if (xmlStrEqual(role, (const xmlChar *)"from") &&
                        xmlStrEqual(type, (const xmlChar *)"way"))
                    {
                        restriction.fromWay = stringToUint((const char *)ref);
                    }
                    if (xmlStrEqual(role, (const xmlChar *)"via") &&
                        xmlStrEqual(type, (const xmlChar *)"node"))
                    {
                        restriction.restriction.viaNode = stringToUint((const char *)ref);
                    }

                    if (NULL != type)
                    {
                        xmlFree(type);
                    }
                    if (NULL != role)
                    {
                        xmlFree(role);
                    }
                    if (NULL != ref)
                    {
                        xmlFree(ref);
                    }
                }
            }
            xmlFree(childName);
        }
    }

    if (ShouldIgnoreRestriction(except_tag_string))
    {
        restriction.fromWay = UINT_MAX; // workaround to ignore the restriction
    }
    return restriction;
}

ExtractionWay XMLParser::ReadXMLWay()
{
    ExtractionWay way;
    if (xmlTextReaderIsEmptyElement(inputReader) == 1)
    {
        return way;
    }
    const int depth = xmlTextReaderDepth(inputReader);
    while (xmlTextReaderRead(inputReader) == 1)
    {
        const int child_type = xmlTextReaderNodeType(inputReader);
        if (child_type != 1 && child_type != 15)
        {
            continue;
        }
        const int childDepth = xmlTextReaderDepth(inputReader);
        xmlChar *childName = xmlTextReaderName(inputReader);
        if (childName == NULL)
        {
            continue;
        }

        if (depth == childDepth && child_type == 15 &&
            xmlStrEqual(childName, (const xmlChar *)"way") == 1)
        {
            xmlChar *id = xmlTextReaderGetAttribute(inputReader, (const xmlChar *)"id");
            way.id = stringToUint((char *)id);
            xmlFree(id);
            xmlFree(childName);
            break;
        }
        if (child_type != 1)
        {
            xmlFree(childName);
            continue;
        }

        if (xmlStrEqual(childName, (const xmlChar *)"tag") == 1)
        {
            xmlChar *k = xmlTextReaderGetAttribute(inputReader, (const xmlChar *)"k");
            xmlChar *value = xmlTextReaderGetAttribute(inputReader, (const xmlChar *)"v");

            if (k != NULL && value != NULL)
            {
                way.keyVals.Add(std::string((char *)k), std::string((char *)value));
            }
            if (k != NULL)
            {
                xmlFree(k);
            }
            if (value != NULL)
            {
                xmlFree(value);
            }
        }
        else if (xmlStrEqual(childName, (const xmlChar *)"nd") == 1)
        {
            xmlChar *ref = xmlTextReaderGetAttribute(inputReader, (const xmlChar *)"ref");
            if (ref != NULL)
            {
                way.path.push_back(stringToUint((const char *)ref));
                xmlFree(ref);
            }
        }
        xmlFree(childName);
    }
    return way;
}

ImportNode XMLParser::ReadXMLNode()
{
    ImportNode node;

    xmlChar *attribute = xmlTextReaderGetAttribute(inputReader, (const xmlChar *)"lat");
    if (attribute != NULL)
    {
        node.lat = COORDINATE_PRECISION * StringToDouble((const char *)attribute);
        xmlFree(attribute);
    }
    attribute = xmlTextReaderGetAttribute(inputReader, (const xmlChar *)"lon");
    if (attribute != NULL)
    {
        node.lon = COORDINATE_PRECISION * StringToDouble((const char *)attribute);
        xmlFree(attribute);
    }
    attribute = xmlTextReaderGetAttribute(inputReader, (const xmlChar *)"id");
    if (attribute != NULL)
    {
        node.id = stringToUint((const char *)attribute);
        xmlFree(attribute);
    }

    if (xmlTextReaderIsEmptyElement(inputReader) == 1)
    {
        return node;
    }
    const int depth = xmlTextReaderDepth(inputReader);
    while (xmlTextReaderRead(inputReader) == 1)
    {
        const int child_type = xmlTextReaderNodeType(inputReader);
        // 1 = Element, 15 = EndElement
        if (child_type != 1 && child_type != 15)
        {
            continue;
        }
        const int childDepth = xmlTextReaderDepth(inputReader);
        xmlChar *childName = xmlTextReaderName(inputReader);
        if (childName == NULL)
        {
            continue;
        }

        if (depth == childDepth && child_type == 15 &&
            xmlStrEqual(childName, (const xmlChar *)"node") == 1)
        {
            xmlFree(childName);
            break;
        }
        if (child_type != 1)
        {
            xmlFree(childName);
            continue;
        }

        if (xmlStrEqual(childName, (const xmlChar *)"tag") == 1)
        {
            xmlChar *k = xmlTextReaderGetAttribute(inputReader, (const xmlChar *)"k");
            xmlChar *value = xmlTextReaderGetAttribute(inputReader, (const xmlChar *)"v");
            if (k != NULL && value != NULL)
            {
                node.keyVals.emplace(std::string((char *)(k)), std::string((char *)(value)));
            }
            if (k != NULL)
            {
                xmlFree(k);
            }
            if (value != NULL)
            {
                xmlFree(value);
            }
        }

        xmlFree(childName);
    }
    return node;
}

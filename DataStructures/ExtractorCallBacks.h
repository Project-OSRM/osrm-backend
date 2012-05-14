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

#ifndef EXTRACTORCALLBACKS_H_
#define EXTRACTORCALLBACKS_H_

#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/regex.hpp>

#include <stxxl.h>
#include "ExtractorStructs.h"

typedef stxxl::vector<NodeID> STXXLNodeIDVector;
typedef stxxl::vector<_Node> STXXLNodeVector;
typedef stxxl::vector<_Edge> STXXLEdgeVector;
typedef stxxl::vector<_Address> STXXLAddressVector;
typedef stxxl::vector<std::string> STXXLStringVector;
typedef stxxl::vector<_RawRestrictionContainer> STXXLRestrictionsVector;
typedef stxxl::vector<_WayIDStartAndEndEdge> STXXLWayIDStartEndVector;


struct STXXLContainers {
    STXXLNodeIDVector           usedNodeIDs;
    STXXLNodeVector             allNodes;
    STXXLEdgeVector             allEdges;
    STXXLAddressVector          adressVector;
    STXXLStringVector           nameVector;
    STXXLRestrictionsVector     restrictionsVector;
    STXXLWayIDStartEndVector    wayStartEndVector;

    STXXLContainers() {
        nameVector.push_back("");
    }

    ~STXXLContainers() {
        usedNodeIDs.clear();
        allNodes.clear();
        allEdges.clear();
        adressVector.clear();
        nameVector.clear();
        restrictionsVector.clear();
        wayStartEndVector.clear();
    }
};

class ExtractorCallbacks{
private:
    Settings settings;
    StringMap * stringMap;
    STXXLContainers * externalMemory;

    struct DurationContainer {
        int hours;
        int minutes;
    };

    bool checkForValidTiming(const std::string &s, DurationContainer & duration) const {
        boost::regex e ("((\\d|\\d\\d):)*(\\d|\\d\\d)",boost::regex_constants::icase|boost::regex_constants::perl);

        std::vector< std::string > result;
        boost::algorithm::split_regex( result, s, boost::regex( ":" ) ) ;
        bool matched = regex_match(s, e);
        if(matched) {
            duration.hours = (result.size()== 2) ?  atoi(result[0].c_str()) : 0;
            duration.minutes = (result.size()== 2) ?  atoi(result[1].c_str()) : atoi(result[0].c_str());
        }
        return matched;
    }
    
    inline int parseMaxspeed(std::string input) const { //call-by-value on purpose.
        boost::algorithm::to_lower(input);
        int n = atoi(input.c_str());
        if (input.find("mph") != std::string::npos || input.find("mp/h") != std::string::npos) {
            n = (n*1609)/1000;
        }
        return n;
    }

public:
    ExtractorCallbacks(STXXLContainers * ext, Settings set, StringMap * strMap) {
        externalMemory = ext;
        settings = set;
        stringMap = strMap;
    }

    ~ExtractorCallbacks() {
    }

    /** warning: caller needs to take care of synchronization! */
    inline bool adressFunction(_Node n, HashTable<std::string, std::string> &keyVals) {
        /*
        std::string housenumber(keyVals.Find("addr:housenumber"));
        std::string housename(keyVals.Find("addr:housename"));
        std::string street(keyVals.Find("addr:street"));
        std::string state(keyVals.Find("addr:state"));
        std::string country(keyVals.Find("addr:country"));
        std::string postcode(keyVals.Find("addr:postcode"));
        std::string city(keyVals.Find("addr:city"));

        if(housenumber != "" || housename != "" || street != "") {
            if(housenumber == "")
                housenumber = housename;
            addressVector->push_back(_Address(n, housenumber, street, state, country, postcode, city));
        }
         */
        return true;
    }

    /** warning: caller needs to take care of synchronization! */
    inline bool nodeFunction(_Node &n) {
        externalMemory->allNodes.push_back(n);
        return true;
    }

    inline bool restrictionFunction(_RawRestrictionContainer &r) {
        externalMemory->restrictionsVector.push_back(r);
        return true;
    }

    /** warning: caller needs to take care of synchronization! */
    inline bool wayFunction(_Way &w) {

        //Get the properties of the way.
        std::string highway( w.keyVals.Find("highway") );
        std::string name( w.keyVals.Find("name") );
        std::string ref( w.keyVals.Find("ref"));
        std::string junction( w.keyVals.Find("junction") );
        std::string route( w.keyVals.Find("route") );
        int maxspeed( parseMaxspeed(w.keyVals.Find("maxspeed")) );
        std::string access( w.keyVals.Find("access") );
        std::string accessTag( w.keyVals.Find(settings.accessTag) );
        std::string man_made( w.keyVals.Find("man_made") );
        std::string barrier( w.keyVals.Find("barrier") );
        std::string oneway( w.keyVals.Find("oneway"));
        std::string onewayClass( w.keyVals.Find("oneway:"+settings.accessTag));
        std::string cycleway( w.keyVals.Find("cycleway"));
        std::string duration ( w.keyVals.Find("duration"));
        std::string service (w.keyVals.Find("service"));
        std::string area(w.keyVals.Find("area"));

        if("yes" == area && settings.ignoreAreas)
            return true;

        //Save the name of the way if it has one, ref has precedence over name tag.
        if ( 0 < ref.length() )
            w.name = ref;
        else
            if ( 0 < name.length() )
                w.name = name;

        if(junction == "roundabout") {
            w.roundabout = true;
        }

        //Is the route tag listed as usable way in the profile?
        if(settings[route] > 0 || settings[man_made] > 0) {
            w.useful = true;
            DurationContainer dc;
            if(checkForValidTiming(duration, dc)){
                w.speed = (600*(dc.hours*60+dc.minutes))/std::max((unsigned)(w.path.size()-1),1u);
                w.isDurationSet = true;
            } else {
                w.speed = settings[route];
            }
            w.direction = _Way::bidirectional;
            if(0 < settings[route])
                highway = route;
            else if (0 < settings[man_made]) {
                highway = man_made;
            }
        }

        //Is the highway tag listed as usable way?
        if(("track" == highway && ("yes" == access || "yes" == accessTag)) || ("track" != highway && (0 < settings[highway] || "yes" == accessTag || "designated" == accessTag) )) {
            if(!w.isDurationSet) {
                if(0 < settings[highway]) {
                    if(0 < maxspeed)
                        if(settings.takeMinimumOfSpeeds)
                            w.speed = std::min(settings[highway], maxspeed);
                        else
                            w.speed = maxspeed;
                    else
                        w.speed = settings[highway];
                } else {
                    if(0 < maxspeed)
                        if(settings.takeMinimumOfSpeeds)
                            w.speed = std::min(settings.defaultSpeed, maxspeed);
                        else w.speed = maxspeed;
                    else
                        w.speed = settings.defaultSpeed;
                    highway = "default";
                }
            }
            w.useful = true;

            //Okay, do we have access to that way?
            if(0 < access.size()) { //fastest way to check for non-empty string
                //If access is forbidden, we don't want to route there.
                if(access == "no" || access == "agricultural" || access == "forestry" || access == "delivery") { //Todo: this is still hard coded
                    w.access = false;
                }
                if(settings.accessRestrictionKeys.find(access) != settings.accessRestrictionKeys.end()) {
                    w.isAccessRestricted = true;
                }
            }

            if(0 < service.size()) {
                if(settings.accessRestrictedService.find(service) != settings.accessRestrictedService.end()) {
                    w.isAccessRestricted = true;
                }
            }

            if("no" == accessTag) {
                return true;
            }

            if( settings.obeyOneways ) {
                if( onewayClass == "yes" || onewayClass == "1" || onewayClass == "true" ) {
                    w.direction = _Way::oneway;
                } else if( onewayClass == "no" || onewayClass == "0" || onewayClass == "false" ) {
                    w.direction = _Way::bidirectional;
                } else if( onewayClass == "-1" ) {
                    w.direction = _Way::opposite;
                } else if( oneway == "no" || oneway == "0" || oneway == "false" ) {
                    w.direction = _Way::bidirectional;
                } else if( settings.accessTag == "bicycle" && (cycleway == "opposite" || cycleway == "opposite_track" || cycleway == "opposite_lane") ) {
                    w.direction = _Way::bidirectional;
                } else if( oneway == "-1") {
                    w.direction  = _Way::opposite;
                } else if( oneway == "yes" || oneway == "1" || oneway == "true" || junction == "roundabout" || highway == "motorway_link" || highway == "motorway" ) {
                    w.direction = _Way::oneway;
                } else {
                    w.direction = _Way::bidirectional;
                }
            } else {
                w.direction = _Way::bidirectional;
            }
        }

        if ( w.useful && w.access && (1 < w.path.size()) ) { //Only true if the way is specified by the speed profile
            w.type = settings.GetHighwayTypeID(highway);
            if(0 > w.type) {
                ERR("Resolved highway " << highway << " to " << w.type);
            }

            //Get the unique identifier for the street name
            const StringMap::const_iterator strit = stringMap->find(w.name);
            if(strit == stringMap->end()) {
                w.nameID = externalMemory->nameVector.size();
                externalMemory->nameVector.push_back(w.name);
                stringMap->insert(StringMap::value_type(w.name, w.nameID));
            } else {
                w.nameID = strit->second;
            }

            if(-1 == w.speed){
                WARN("found way with bogus speed, id: " << w.id);
                return true;
            }
            if(w.id == UINT_MAX) {
                WARN("found way with unknown type: " << w.id);
                return true;
            }

            if ( w.direction == _Way::opposite ){
                std::reverse( w.path.begin(), w.path.end() );
            }

            for(vector< NodeID >::size_type n = 0; n < w.path.size()-1; ++n) {
                externalMemory->allEdges.push_back(_Edge(w.path[n], w.path[n+1], w.type, w.direction, w.speed, w.nameID, w.roundabout, highway == settings.excludeFromGrid || "pier" == highway, w.isDurationSet, w.isAccessRestricted));
                externalMemory->usedNodeIDs.push_back(w.path[n]);
            }
            externalMemory->usedNodeIDs.push_back(w.path[w.path.size()-1]);

            //The following information is needed to identify start and end segments of restrictions
            externalMemory->wayStartEndVector.push_back(_WayIDStartAndEndEdge(w.id, w.path[0], w.path[1], w.path[w.path.size()-2], w.path[w.path.size()-1]));
        }
        return true;
    }
};

#endif /* EXTRACTORCALLBACKS_H_ */

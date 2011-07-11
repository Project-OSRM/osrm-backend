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

#ifndef GPX_DESCRIPTOR_H_
#define GPX_DESCRIPTOR_H_

#include "BaseDescriptor.h"

template<class SearchEngineT>
class GPXDescriptor : public BaseDescriptor<SearchEngineT>{
private:
    _DescriptorConfig config;
    string tmp;
    _Coordinate current;
public:
    void SetConfig(const _DescriptorConfig& c) { config = c; }
    void Run(http::Reply& reply, RawRouteData * route, PhantomNodes * phantomNodes, SearchEngineT * sEngine, unsigned distance) {
        reply.content += ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
        reply.content += "<gpx xmlns=\"http://www.topografix.com/GPX/1/1\" "
                              "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
                              "xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 gpx.xsd"
                              "\">";
        reply.content += "<rte>";
        if(distance != UINT_MAX && route->routeSegments.size()) {

            convertInternalLatLonToString(phantomNodes->startPhantom.location.lat, tmp);
            reply.content += "<rtept lat=\"" + tmp + "\" ";
            convertInternalLatLonToString(phantomNodes->startPhantom.location.lon, tmp);
            reply.content += "lon=\"" + tmp + "\"></rtept>";

            for(unsigned segmentIdx = 0; segmentIdx < route->routeSegments.size(); segmentIdx++) {
                const std::vector< _PathData > & path = route->routeSegments[segmentIdx];
                if( ! path.size() )
                    continue;
                for(vector< _PathData >::const_iterator it = path.begin(); it != path.end(); it++) {
                    sEngine->getCoordinatesForNodeID(it->node, current);

                    convertInternalLatLonToString(current.lat, tmp);
                    reply.content += "<rtept lat=\"" + tmp + "\" ";
                    convertInternalLatLonToString(current.lon, tmp);
                    reply.content += "lon=\"" + tmp + "\">";

                    reply.content +="</rtept>";
                }
            }
            convertInternalLatLonToString(phantomNodes->targetPhantom.location.lat, tmp);
            reply.content += "<rtept lat=\"" + tmp + "\" ";
            convertInternalLatLonToString(phantomNodes->targetPhantom.location.lon, tmp);
            reply.content += "lon=\"" + tmp + "\"></rtept>";
        }
        reply.content += "</rte></gpx>";
    }
};
#endif /* GPX_DESCRIPTOR_H_ */

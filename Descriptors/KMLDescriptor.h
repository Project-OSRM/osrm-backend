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

#ifndef KML_DESCRIPTOR_H_
#define KML_DESCRIPTOR_H_

#include <boost/foreach.hpp>
#include "BaseDescriptor.h"

template<class SearchEngineT>
class KMLDescriptor : public BaseDescriptor<SearchEngineT>{
private:
    _DescriptorConfig config;
    _Coordinate current;

    std::string tmp;
public:
    void SetConfig(const _DescriptorConfig& c) { config = c; }
    void Run(http::Reply & reply, RawRouteData &rawRoute, PhantomNodes &phantomNodes, SearchEngineT &sEngine, unsigned distance) {
        reply.content += ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
        reply.content += "<kml xmlns=\"http://www.opengis.net/kml/2.2\">";
        reply.content += "<Document><name>OSRM route</name><Placemark><name>OSRM projected route</name>";
        reply.content += "<LineString><altitudeMode>relativeToGround</altitudeMode><coordinates>";

        if(distance != UINT_MAX && rawRoute.computedRouted.size()) {
            convertInternalLatLonToString(phantomNodes.startPhantom.location.lon, tmp);
            reply.content += tmp + ",";
            convertInternalLatLonToString(phantomNodes.startPhantom.location.lat, tmp);
            reply.content += tmp + ",0";  // KML expects also hieght. We fixed to zero and use 'relativeToGround' altitude mode

            BOOST_FOREACH(_PathData pathData, rawRoute.computedRouted) {
                sEngine.GetCoordinatesForNodeID(pathData.node, current);

                convertInternalLatLonToString(current.lon, tmp);
                reply.content += " " + tmp + ",";
                convertInternalLatLonToString(current.lat, tmp);
                reply.content += tmp + ",0";
            }
            convertInternalLatLonToString(phantomNodes.targetPhantom.location.lon, tmp);
            reply.content += " " + tmp + ",";
            convertInternalLatLonToString(phantomNodes.targetPhantom.location.lat, tmp);
            reply.content += tmp + ",0";
        }
        reply.content += "</coordinates></LineString></Placemark></Document></kml>";
    }
};
#endif /* KML_DESCRIPTOR_H_ */

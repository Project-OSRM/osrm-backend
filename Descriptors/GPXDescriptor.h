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

#ifndef GPX_DESCRIPTOR_H_
#define GPX_DESCRIPTOR_H_

#include "BaseDescriptor.h"

#include <boost/foreach.hpp>

template<class DataFacadeT>
class GPXDescriptor : public BaseDescriptor<DataFacadeT> {
private:
    DescriptorConfig config;
    FixedPointCoordinate current;

    std::string tmp;
public:
    void SetConfig(const DescriptorConfig & c) { config = c; }

    //TODO: reorder parameters
    void Run(
        http::Reply & reply,
        const RawRouteData &rawRoute,
        PhantomNodes &phantomNodes,
        const DataFacadeT * facade
    ) {
        reply.content.push_back("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
        reply.content.push_back(
                "<gpx creator=\"OSRM Routing Engine\" version=\"1.1\" "
                "xmlns=\"http://www.topografix.com/GPX/1/1\" "
                "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
                "xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 gpx.xsd"
                "\">");
        reply.content.push_back(
                "<metadata><copyright author=\"Project OSRM\"><license>Data (c)"
                " OpenStreetMap contributors (ODbL)</license></copyright>"
                "</metadata>");
        reply.content.push_back("<rte>");
        bool found_route =  (rawRoute.lengthOfShortestPath != INT_MAX) &&
                            (rawRoute.computedShortestPath.size()         );
        if( found_route ) {
            convertInternalLatLonToString(
                phantomNodes.startPhantom.location.lat,
                tmp
            );
            reply.content.push_back("<rtept lat=\"" + tmp + "\" ");
            convertInternalLatLonToString(
                phantomNodes.startPhantom.location.lon,
                tmp
            );
            reply.content.push_back("lon=\"" + tmp + "\"></rtept>");

            BOOST_FOREACH(
                const _PathData & pathData,
                rawRoute.computedShortestPath
            ) {
                current = facade->GetCoordinateOfNode(pathData.node);

                convertInternalLatLonToString(current.lat, tmp);
                reply.content.push_back("<rtept lat=\"" + tmp + "\" ");
                convertInternalLatLonToString(current.lon, tmp);
                reply.content.push_back("lon=\"" + tmp + "\"></rtept>");
            }
            convertInternalLatLonToString(
                phantomNodes.targetPhantom.location.lat,
                tmp
            );
            reply.content.push_back("<rtept lat=\"" + tmp + "\" ");
            convertInternalLatLonToString(
                phantomNodes.targetPhantom.location.lon,
                tmp
            );
            reply.content.push_back("lon=\"" + tmp + "\"></rtept>");
        }
        reply.content.push_back("</rte></gpx>");
    }
};
#endif // GPX_DESCRIPTOR_H_

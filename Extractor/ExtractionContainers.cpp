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

#include "ExtractionContainers.h"

void ExtractionContainers::PrepareData(const std::string & output_file_name, const std::string restrictionsFileName, const unsigned amountOfRAM) {
    try {
        unsigned usedNodeCounter = 0;
        unsigned usedEdgeCounter = 0;
        double time = get_timestamp();
        boost::uint64_t memory_to_use = static_cast<boost::uint64_t>(amountOfRAM) * 1024 * 1024 * 1024;

        std::cout << "[extractor] Sorting used nodes        ... " << std::flush;
        stxxl::sort(usedNodeIDs.begin(), usedNodeIDs.end(), Cmp(), memory_to_use);
        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;

        time = get_timestamp();
        std::cout << "[extractor] Erasing duplicate nodes   ... " << std::flush;
        stxxl::vector<NodeID>::iterator NewEnd = std::unique ( usedNodeIDs.begin(),usedNodeIDs.end() ) ;
        usedNodeIDs.resize ( NewEnd - usedNodeIDs.begin() );
        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;
        time = get_timestamp();

        std::cout << "[extractor] Sorting all nodes         ... " << std::flush;
        stxxl::sort(allNodes.begin(), allNodes.end(), CmpNodeByID(), memory_to_use);
        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;
        time = get_timestamp();

        std::cout << "[extractor] Sorting used ways         ... " << std::flush;
        stxxl::sort(wayStartEndVector.begin(), wayStartEndVector.end(), CmpWayByID(), memory_to_use);
        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;

        std::cout << "[extractor] Sorting restrctns. by from... " << std::flush;
        stxxl::sort(restrictionsVector.begin(), restrictionsVector.end(), CmpRestrictionContainerByFrom(), memory_to_use);
        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;

        std::cout << "[extractor] Fixing restriction starts ... " << std::flush;
        STXXLRestrictionsVector::iterator restrictionsIT = restrictionsVector.begin();
        STXXLWayIDStartEndVector::iterator wayStartAndEndEdgeIT = wayStartEndVector.begin();

        while(wayStartAndEndEdgeIT != wayStartEndVector.end() && restrictionsIT != restrictionsVector.end()) {
            if(wayStartAndEndEdgeIT->wayID < restrictionsIT->fromWay){
                ++wayStartAndEndEdgeIT;
                continue;
            }
            if(wayStartAndEndEdgeIT->wayID > restrictionsIT->fromWay) {
                ++restrictionsIT;
                continue;
            }
            assert(wayStartAndEndEdgeIT->wayID == restrictionsIT->fromWay);
            NodeID viaNode = restrictionsIT->restriction.viaNode;

            if(wayStartAndEndEdgeIT->firstStart == viaNode) {
                restrictionsIT->restriction.fromNode = wayStartAndEndEdgeIT->firstTarget;
            } else if(wayStartAndEndEdgeIT->firstTarget == viaNode) {
                restrictionsIT->restriction.fromNode = wayStartAndEndEdgeIT->firstStart;
            } else if(wayStartAndEndEdgeIT->lastStart == viaNode) {
                restrictionsIT->restriction.fromNode = wayStartAndEndEdgeIT->lastTarget;
            } else if(wayStartAndEndEdgeIT->lastTarget == viaNode) {
                restrictionsIT->restriction.fromNode = wayStartAndEndEdgeIT->lastStart;
            }
            ++restrictionsIT;
        }

        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;
        time = get_timestamp();

        std::cout << "[extractor] Sorting restrctns. by to  ... " << std::flush;
        stxxl::sort(restrictionsVector.begin(), restrictionsVector.end(), CmpRestrictionContainerByTo(), memory_to_use);
        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;

        time = get_timestamp();
        unsigned usableRestrictionsCounter(0);
        std::cout << "[extractor] Fixing restriction ends   ... " << std::flush;
        restrictionsIT = restrictionsVector.begin();
        wayStartAndEndEdgeIT = wayStartEndVector.begin();
        while(wayStartAndEndEdgeIT != wayStartEndVector.end() && restrictionsIT != restrictionsVector.end()) {
            if(wayStartAndEndEdgeIT->wayID < restrictionsIT->toWay){
                ++wayStartAndEndEdgeIT;
                continue;
            }
            if(wayStartAndEndEdgeIT->wayID > restrictionsIT->toWay) {
                ++restrictionsIT;
                continue;
            }
            NodeID viaNode = restrictionsIT->restriction.viaNode;
            if(wayStartAndEndEdgeIT->lastStart == viaNode) {
                restrictionsIT->restriction.toNode = wayStartAndEndEdgeIT->lastTarget;
            } else if(wayStartAndEndEdgeIT->lastTarget == viaNode) {
                restrictionsIT->restriction.toNode = wayStartAndEndEdgeIT->lastStart;
            } else if(wayStartAndEndEdgeIT->firstStart == viaNode) {
                restrictionsIT->restriction.toNode = wayStartAndEndEdgeIT->firstTarget;
            } else if(wayStartAndEndEdgeIT->firstTarget == viaNode) {
                restrictionsIT->restriction.toNode = wayStartAndEndEdgeIT->firstStart;
            }

            if(
                UINT_MAX != restrictionsIT->restriction.fromNode &&
                UINT_MAX != restrictionsIT->restriction.toNode
            ) {
                ++usableRestrictionsCounter;
            }
            ++restrictionsIT;
        }
        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;
        SimpleLogger().Write() << "usable restrictions: " << usableRestrictionsCounter;
        //serialize restrictions
        std::ofstream restrictionsOutstream;
        restrictionsOutstream.open(restrictionsFileName.c_str(), std::ios::binary);
        restrictionsOutstream.write((char*)&uuid, sizeof(UUID));
        restrictionsOutstream.write((char*)&usableRestrictionsCounter, sizeof(unsigned));
        for(
            restrictionsIT = restrictionsVector.begin();
            restrictionsIT != restrictionsVector.end();
            ++restrictionsIT
        ) {
            if(
                UINT_MAX != restrictionsIT->restriction.fromNode &&
                UINT_MAX != restrictionsIT->restriction.toNode
            ) {
                restrictionsOutstream.write((char *)&(restrictionsIT->restriction), sizeof(TurnRestriction));
            }
        }
        restrictionsOutstream.close();

        std::ofstream fout;
        fout.open(output_file_name.c_str(), std::ios::binary);
        fout.write((char*)&uuid, sizeof(UUID));
        fout.write((char*)&usedNodeCounter, sizeof(unsigned));
        time = get_timestamp();
        std::cout << "[extractor] Confirming/Writing used nodes     ... " << std::flush;

        STXXLNodeVector::iterator nodesIT = allNodes.begin();
        STXXLNodeIDVector::iterator usedNodeIDsIT = usedNodeIDs.begin();
        while(usedNodeIDsIT != usedNodeIDs.end() && nodesIT != allNodes.end()) {
            if(*usedNodeIDsIT < nodesIT->id){
                ++usedNodeIDsIT;
                continue;
            }
            if(*usedNodeIDsIT > nodesIT->id) {
                ++nodesIT;
                continue;
            }
            if(*usedNodeIDsIT == nodesIT->id) {
                fout.write((char*)&(*nodesIT), sizeof(_Node));
                ++usedNodeCounter;
                ++usedNodeIDsIT;
                ++nodesIT;
            }
        }

        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;

        std::cout << "[extractor] setting number of nodes   ... " << std::flush;
        std::ios::pos_type positionInFile = fout.tellp();
        fout.seekp(std::ios::beg+sizeof(UUID));
        fout.write((char*)&usedNodeCounter, sizeof(unsigned));
        fout.seekp(positionInFile);

        std::cout << "ok" << std::endl;
        time = get_timestamp();

        // Sort edges by start.
        std::cout << "[extractor] Sorting edges by start    ... " << std::flush;
        stxxl::sort(allEdges.begin(), allEdges.end(), CmpEdgeByStartID(), memory_to_use);
        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;
        time = get_timestamp();

        std::cout << "[extractor] Setting start coords      ... " << std::flush;
        fout.write((char*)&usedEdgeCounter, sizeof(unsigned));
        // Traverse list of edges and nodes in parallel and set start coord
        nodesIT = allNodes.begin();
        STXXLEdgeVector::iterator edgeIT = allEdges.begin();
        while(edgeIT != allEdges.end() && nodesIT != allNodes.end()) {
            if(edgeIT->start < nodesIT->id){
                ++edgeIT;
                continue;
            }
            if(edgeIT->start > nodesIT->id) {
                nodesIT++;
                continue;
            }
            if(edgeIT->start == nodesIT->id) {
                edgeIT->startCoord.lat = nodesIT->lat;
                edgeIT->startCoord.lon = nodesIT->lon;
                ++edgeIT;
            }
        }
        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;
        time = get_timestamp();

        // Sort Edges by target
        std::cout << "[extractor] Sorting edges by target   ... " << std::flush;
        stxxl::sort(allEdges.begin(), allEdges.end(), CmpEdgeByTargetID(), memory_to_use);
        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;
        time = get_timestamp();

        std::cout << "[extractor] Setting target coords     ... " << std::flush;
        // Traverse list of edges and nodes in parallel and set target coord
        nodesIT = allNodes.begin();
        edgeIT = allEdges.begin();

        while(edgeIT != allEdges.end() && nodesIT != allNodes.end()) {
            if(edgeIT->target < nodesIT->id){
                ++edgeIT;
                continue;
            }
            if(edgeIT->target > nodesIT->id) {
                ++nodesIT;
                continue;
            }
            if(edgeIT->target == nodesIT->id) {
                if(edgeIT->startCoord.lat != INT_MIN && edgeIT->startCoord.lon != INT_MIN) {
                    edgeIT->targetCoord.lat = nodesIT->lat;
                    edgeIT->targetCoord.lon = nodesIT->lon;

                    double distance = ApproximateDistance(edgeIT->startCoord.lat, edgeIT->startCoord.lon, nodesIT->lat, nodesIT->lon);
                    assert(edgeIT->speed != -1);
                    double weight = ( distance * 10. ) / (edgeIT->speed / 3.6);
                    int intWeight = std::max(1, (int)std::floor((edgeIT->isDurationSet ? edgeIT->speed : weight)+.5) );
                    int intDist = std::max(1, (int)distance);
                    short zero = 0;
                    short one = 1;

                    fout.write((char*)&edgeIT->start, sizeof(unsigned));
                    fout.write((char*)&edgeIT->target, sizeof(unsigned));
                    fout.write((char*)&intDist, sizeof(int));
                    switch(edgeIT->direction) {
                    case ExtractionWay::notSure:
                        fout.write((char*)&zero, sizeof(short));
                        break;
                    case ExtractionWay::oneway:
                        fout.write((char*)&one, sizeof(short));
                        break;
                    case ExtractionWay::bidirectional:
                        fout.write((char*)&zero, sizeof(short));

                        break;
                    case ExtractionWay::opposite:
                        fout.write((char*)&one, sizeof(short));
                        break;
                    default:
                      std::cerr << "[error] edge with no direction: " << edgeIT->direction << std::endl;
                      assert(false);
                        break;
                    }
                    fout.write((char*)&intWeight, sizeof(int));
                    assert(edgeIT->type >= 0);
                    fout.write((char*)&edgeIT->type, sizeof(short));
                    fout.write((char*)&edgeIT->nameID, sizeof(unsigned));
                    fout.write((char*)&edgeIT->isRoundabout, sizeof(bool));
                    fout.write((char*)&edgeIT->ignoreInGrid, sizeof(bool));
                    fout.write((char*)&edgeIT->isAccessRestricted, sizeof(bool));
                    fout.write((char*)&edgeIT->isContraFlow, sizeof(bool));
                    ++usedEdgeCounter;
                }
                ++edgeIT;
            }
        }
        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;
        std::cout << "[extractor] setting number of edges   ... " << std::flush;

        fout.seekp(positionInFile);
        fout.write((char*)&usedEdgeCounter, sizeof(unsigned));
        fout.close();
        std::cout << "ok" << std::endl;
        time = get_timestamp();

        std::cout << "[extractor] writing street name index ... " << std::flush;
        std::string name_file_streamName = (output_file_name + ".names");
        boost::filesystem::ofstream name_file_stream(
            name_file_streamName,
            std::ios::binary
        );

        //write number of names
        const unsigned number_of_names = name_list.size()+1;
        name_file_stream.write((char *)&(number_of_names), sizeof(unsigned));

        //compute total number of chars
        unsigned total_number_of_chars = 0;
        BOOST_FOREACH(const std::string & str, name_list) {
            total_number_of_chars += strlen(str.c_str());
        }
        //write total number of chars
        name_file_stream.write(
            (char *)&(total_number_of_chars),
            sizeof(unsigned)
        );
        //write prefixe sums
        unsigned name_lengths_prefix_sum = 0;
        BOOST_FOREACH(const std::string & str, name_list) {
            name_file_stream.write(
                (char *)&(name_lengths_prefix_sum),
                sizeof(unsigned)
            );
            name_lengths_prefix_sum += strlen(str.c_str());
        }
        //duplicate on purpose!
        name_file_stream.write(
            (char *)&(name_lengths_prefix_sum),
            sizeof(unsigned)
        );

        //write all chars consecutively
        BOOST_FOREACH(const std::string & str, name_list) {
            const unsigned lengthOfRawString = strlen(str.c_str());
            name_file_stream.write(str.c_str(), lengthOfRawString);
        }

        name_file_stream.close();
        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;
        SimpleLogger().Write() <<
            "Processed " << usedNodeCounter << " nodes and " << usedEdgeCounter << " edges";


    } catch ( const std::exception& e ) {
        std::cerr <<  "Caught Execption:" << e.what() << std::endl;
    }
}


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

#include "ExtractionContainers.h"

void ExtractionContainers::PrepareData(const std::string & outputFileName, const std::string restrictionsFileName, const unsigned amountOfRAM) {
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

            if(UINT_MAX != restrictionsIT->restriction.fromNode && UINT_MAX != restrictionsIT->restriction.toNode) {
                ++usableRestrictionsCounter;
            }
            ++restrictionsIT;
        }
        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;
        INFO("usable restrictions: " << usableRestrictionsCounter );
        //serialize restrictions
        std::ofstream restrictionsOutstream;
        restrictionsOutstream.open(restrictionsFileName.c_str(), std::ios::binary);
        restrictionsOutstream.write((char*)&usableRestrictionsCounter, sizeof(unsigned));
        for(restrictionsIT = restrictionsVector.begin(); restrictionsIT != restrictionsVector.end(); ++restrictionsIT) {
            if(UINT_MAX != restrictionsIT->restriction.fromNode && UINT_MAX != restrictionsIT->restriction.toNode) {
                restrictionsOutstream.write((char *)&(restrictionsIT->restriction), sizeof(_Restriction));
            }
        }
        restrictionsOutstream.close();

        std::ofstream fout;
        fout.open(outputFileName.c_str(), std::ios::binary);
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
        fout.seekp(std::ios::beg);
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
                    fout.write((char*)&edgeIT->nameID, sizeof(unsigned));
                    fout.write((char*)&edgeIT->isRoundabout, sizeof(bool));
                    fout.write((char*)&edgeIT->ignoreInGrid, sizeof(bool));
                    fout.write((char*)&edgeIT->isAccessRestricted, sizeof(bool));
                    fout.write((char*)&edgeIT->mode, sizeof(unsigned char));
                }
                ++usedEdgeCounter;
                ++edgeIT;
            }
        }
        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;
        std::cout << "[extractor] setting number of edges   ... " << std::flush;

        std::cout << "[extractor]  number of edges:    " << usedEdgeCounter << std::flush;

        fout.seekp(positionInFile);
        fout.write((char*)&usedEdgeCounter, sizeof(unsigned));
        fout.close();
        std::cout << "ok" << std::endl;
        time = get_timestamp();
        std::cout << "[extractor] writing street name index ... " << std::flush;
        std::string nameOutFileName = (outputFileName + ".names");
        std::ofstream nameOutFile(nameOutFileName.c_str(), std::ios::binary);
        unsigned sizeOfNameIndex = nameVector.size();
        nameOutFile.write((char *)&(sizeOfNameIndex), sizeof(unsigned));

        BOOST_FOREACH(const std::string & str, nameVector) {
            unsigned lengthOfRawString = strlen(str.c_str());
            nameOutFile.write((char *)&(lengthOfRawString), sizeof(unsigned));
            nameOutFile.write(str.c_str(), lengthOfRawString);
        }

        nameOutFile.close();
        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;

        //        time = get_timestamp();
        //        cout << "[extractor] writing address list      ... " << flush;
        //
        //        adressFileName.append(".address");
        //        ofstream addressOutFile(adressFileName.c_str());
        //        for(STXXLAddressVector::iterator it = adressVector.begin(); it != adressVector.end(); it++) {
        //            addressOutFile << it->node.id << "|" << it->node.lat << "|" << it->node.lon << "|" << it->city << "|" << it->street << "|" << it->housenumber << "|" << it->state << "|" << it->country << "\n";
        //        }
        //        addressOutFile.close();
        //        cout << "ok, after " << get_timestamp() - time << "s" << endl;

        INFO("Processed " << usedNodeCounter << " nodes and " << usedEdgeCounter << " edges");


    } catch ( const std::exception& e ) {
      std::cerr <<  "Caught Execption:" << e.what() << std::endl;
    }
}


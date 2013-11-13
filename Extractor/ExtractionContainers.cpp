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

void ExtractionContainers::PrepareData(
    const std::string & output_file_name,
    const std::string & restrictions_file_name
) {
    try {
        unsigned number_of_used_nodes = 0;
        unsigned number_of_used_edges = 0;
        double time = get_timestamp();

        std::cout << "[extractor] Sorting used nodes        ... " << std::flush;
        stxxl::sort(
            used_node_id_list.begin(),
            used_node_id_list.end(),
            Cmp(),
            4294967296
        );
        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;

        time = get_timestamp();
        std::cout << "[extractor] Erasing duplicate nodes   ... " << std::flush;
        stxxl::vector<NodeID>::iterator NewEnd = std::unique ( used_node_id_list.begin(),used_node_id_list.end() ) ;
        used_node_id_list.resize ( NewEnd - used_node_id_list.begin() );
        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;
        time = get_timestamp();

        std::cout << "[extractor] Sorting all nodes         ... " << std::flush;
        stxxl::sort(
            all_nodes_list.begin(),
            all_nodes_list.end(),
            CmpNodeByID(),
            4294967296
        );
        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;
        time = get_timestamp();

        std::cout << "[extractor] Sorting used ways         ... " << std::flush;
        stxxl::sort(
            way_start_end_id_list.begin(),
            way_start_end_id_list.end(),
            CmpWayByID(),
            4294967296
        );
        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;

        std::cout << "[extractor] Sorting restrctns. by from... " << std::flush;
        stxxl::sort(
            restrictions_list.begin(),
            restrictions_list.end(),
            CmpRestrictionContainerByFrom(),
            4294967296
        );
        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;

        std::cout << "[extractor] Fixing restriction starts ... " << std::flush;
        STXXLRestrictionsVector::iterator restrictions_iterator = restrictions_list.begin();
        STXXLWayIDStartEndVector::iterator way_start_and_end_iterator = way_start_end_id_list.begin();

        while(
            way_start_and_end_iterator != way_start_end_id_list.end() &&
            restrictions_iterator      != restrictions_list.end()
        ) {

            if(way_start_and_end_iterator->wayID < restrictions_iterator->fromWay){
                ++way_start_and_end_iterator;
                continue;
            }

            if(way_start_and_end_iterator->wayID > restrictions_iterator->fromWay) {
                ++restrictions_iterator;
                continue;
            }

            BOOST_ASSERT(way_start_and_end_iterator->wayID == restrictions_iterator->fromWay);
            NodeID via_node_id = restrictions_iterator->restriction.viaNode;

            if(way_start_and_end_iterator->firstStart == via_node_id) {
                restrictions_iterator->restriction.fromNode = way_start_and_end_iterator->firstTarget;
            } else if(way_start_and_end_iterator->firstTarget == via_node_id) {
                restrictions_iterator->restriction.fromNode = way_start_and_end_iterator->firstStart;
            } else if(way_start_and_end_iterator->lastStart == via_node_id) {
                restrictions_iterator->restriction.fromNode = way_start_and_end_iterator->lastTarget;
            } else if(way_start_and_end_iterator->lastTarget == via_node_id) {
                restrictions_iterator->restriction.fromNode = way_start_and_end_iterator->lastStart;
            }
            ++restrictions_iterator;
        }

        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;
        time = get_timestamp();

        std::cout << "[extractor] Sorting restrctns. by to  ... " << std::flush;
        stxxl::sort(
            restrictions_list.begin(),
            restrictions_list.end(),
            CmpRestrictionContainerByTo(),
            4294967296
        );
        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;

        time = get_timestamp();
        unsigned usableRestrictionsCounter(0);
        std::cout << "[extractor] Fixing restriction ends   ... " << std::flush;
        restrictions_iterator = restrictions_list.begin();
        way_start_and_end_iterator = way_start_end_id_list.begin();
        while(
            way_start_and_end_iterator != way_start_end_id_list.end() &&
            restrictions_iterator       != restrictions_list.end()
        ) {
            if(way_start_and_end_iterator->wayID < restrictions_iterator->toWay){
                ++way_start_and_end_iterator;
                continue;
            }
            if(way_start_and_end_iterator->wayID > restrictions_iterator->toWay) {
                ++restrictions_iterator;
                continue;
            }
            NodeID via_node_id = restrictions_iterator->restriction.viaNode;
            if(way_start_and_end_iterator->lastStart == via_node_id) {
                restrictions_iterator->restriction.toNode = way_start_and_end_iterator->lastTarget;
            } else if(way_start_and_end_iterator->lastTarget == via_node_id) {
                restrictions_iterator->restriction.toNode = way_start_and_end_iterator->lastStart;
            } else if(way_start_and_end_iterator->firstStart == via_node_id) {
                restrictions_iterator->restriction.toNode = way_start_and_end_iterator->firstTarget;
            } else if(way_start_and_end_iterator->firstTarget == via_node_id) {
                restrictions_iterator->restriction.toNode = way_start_and_end_iterator->firstStart;
            }

            if(
                UINT_MAX != restrictions_iterator->restriction.fromNode &&
                UINT_MAX != restrictions_iterator->restriction.toNode
            ) {
                ++usableRestrictionsCounter;
            }
            ++restrictions_iterator;
        }
        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;
        SimpleLogger().Write() << "usable restrictions: " << usableRestrictionsCounter;
        //serialize restrictions
        std::ofstream restrictions_out_stream;
        restrictions_out_stream.open(restrictions_file_name.c_str(), std::ios::binary);
        restrictions_out_stream.write((char*)&uuid, sizeof(UUID));
        restrictions_out_stream.write(
            (char*)&usableRestrictionsCounter,
            sizeof(unsigned)
        );
        for(
            restrictions_iterator = restrictions_list.begin();
            restrictions_iterator != restrictions_list.end();
            ++restrictions_iterator
        ) {
            if(
                UINT_MAX != restrictions_iterator->restriction.fromNode &&
                UINT_MAX != restrictions_iterator->restriction.toNode
            ) {
                restrictions_out_stream.write(
                    (char *)&(restrictions_iterator->restriction),
                    sizeof(TurnRestriction)
                );
            }
        }
        restrictions_out_stream.close();

        std::ofstream file_out_stream;
        file_out_stream.open(output_file_name.c_str(), std::ios::binary);
        file_out_stream.write((char*)&uuid, sizeof(UUID));
        file_out_stream.write((char*)&number_of_used_nodes, sizeof(unsigned));
        time = get_timestamp();
        std::cout << "[extractor] Confirming/Writing used nodes     ... " << std::flush;

        //identify all used nodes by a merging step of two sorted lists
        STXXLNodeVector::iterator node_iterator = all_nodes_list.begin();
        STXXLNodeIDVector::iterator node_id_iterator = used_node_id_list.begin();
        while(
            node_id_iterator != used_node_id_list.end() &&
            node_iterator    != all_nodes_list.end()
        ) {
            if(*node_id_iterator < node_iterator->id){
                ++node_id_iterator;
                continue;
            }
            if(*node_id_iterator > node_iterator->id) {
                ++node_iterator;
                continue;
            }
            BOOST_ASSERT( *node_id_iterator == node_iterator->id);

            file_out_stream.write(
                (char*)&(*node_iterator),
                sizeof(ExternalMemoryNode)
            );

            ++number_of_used_nodes;
            ++node_id_iterator;
            ++node_iterator;
        }

        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;

        std::cout << "[extractor] setting number of nodes   ... " << std::flush;
        std::ios::pos_type previous_file_position = file_out_stream.tellp();
        file_out_stream.seekp(std::ios::beg+sizeof(UUID));
        file_out_stream.write((char*)&number_of_used_nodes, sizeof(unsigned));
        file_out_stream.seekp(previous_file_position);

        std::cout << "ok" << std::endl;
        time = get_timestamp();

        // Sort edges by start.
        std::cout << "[extractor] Sorting edges by start    ... " << std::flush;
        stxxl::sort(
            all_edges_list.begin(),
            all_edges_list.end(),
            CmpEdgeByStartID(),
            4294967296
        );
        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;
        time = get_timestamp();

        std::cout << "[extractor] Setting start coords      ... " << std::flush;
        file_out_stream.write((char*)&number_of_used_edges, sizeof(unsigned));
        // Traverse list of edges and nodes in parallel and set start coord
        node_iterator = all_nodes_list.begin();
        STXXLEdgeVector::iterator edge_iterator = all_edges_list.begin();
        while(
            edge_iterator != all_edges_list.end() &&
            node_iterator != all_nodes_list.end()
        ) {
            if(edge_iterator->start < node_iterator->id){
                ++edge_iterator;
                continue;
            }
            if(edge_iterator->start > node_iterator->id) {
                node_iterator++;
                continue;
            }

            BOOST_ASSERT(edge_iterator->start == node_iterator->id);
            edge_iterator->startCoord.lat = node_iterator->lat;
            edge_iterator->startCoord.lon = node_iterator->lon;
            ++edge_iterator;
        }
        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;
        time = get_timestamp();

        // Sort Edges by target
        std::cout << "[extractor] Sorting edges by target   ... " << std::flush;
        stxxl::sort(
            all_edges_list.begin(),
            all_edges_list.end(),
            CmpEdgeByTargetID(),
            4294967296
        );
        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;
        time = get_timestamp();

        std::cout << "[extractor] Setting target coords     ... " << std::flush;
        // Traverse list of edges and nodes in parallel and set target coord
        node_iterator = all_nodes_list.begin();
        edge_iterator = all_edges_list.begin();

        while(
            edge_iterator != all_edges_list.end() &&
            node_iterator != all_nodes_list.end()
        ) {
            if(edge_iterator->target < node_iterator->id){
                ++edge_iterator;
                continue;
            }
            if(edge_iterator->target > node_iterator->id) {
                ++node_iterator;
                continue;
            }
            BOOST_ASSERT(edge_iterator->target == node_iterator->id);
            if(edge_iterator->startCoord.lat != INT_MIN && edge_iterator->startCoord.lon != INT_MIN) {
                edge_iterator->targetCoord.lat = node_iterator->lat;
                edge_iterator->targetCoord.lon = node_iterator->lon;

                const double distance = ApproximateDistance(
                    edge_iterator->startCoord.lat,
                    edge_iterator->startCoord.lon,
                    node_iterator->lat,
                    node_iterator->lon
                );

                BOOST_ASSERT(edge_iterator->speed != -1);
                const double weight = ( distance * 10. ) / (edge_iterator->speed / 3.6);
                int integer_weight = std::max( 1, (int)std::floor((edge_iterator->isDurationSet ? edge_iterator->speed : weight)+.5) );
                int integer_distance = std::max( 1, (int)distance );
                short zero = 0;
                short one = 1;

                file_out_stream.write((char*)&edge_iterator->start, sizeof(unsigned));
                file_out_stream.write((char*)&edge_iterator->target, sizeof(unsigned));
                file_out_stream.write((char*)&integer_distance, sizeof(int));
                switch(edge_iterator->direction) {
                case ExtractionWay::notSure:
                    file_out_stream.write((char*)&zero, sizeof(short));
                    break;
                case ExtractionWay::oneway:
                    file_out_stream.write((char*)&one, sizeof(short));
                    break;
                case ExtractionWay::bidirectional:
                    file_out_stream.write((char*)&zero, sizeof(short));

                    break;
                case ExtractionWay::opposite:
                    file_out_stream.write((char*)&one, sizeof(short));
                    break;
                default:
                    throw OSRMException("edge has broken direction");
                    break;
                }
                file_out_stream.write(
                    (char*)&integer_weight, sizeof(int)
                );
                BOOST_ASSERT(edge_iterator->type >= 0);
                file_out_stream.write(
                    (char*)&edge_iterator->type,
                    sizeof(short)
                );
                file_out_stream.write(
                    (char*)&edge_iterator->nameID,
                    sizeof(unsigned)
                );
                file_out_stream.write(
                    (char*)&edge_iterator->isRoundabout,
                    sizeof(bool)
                );
                file_out_stream.write(
                    (char*)&edge_iterator->ignoreInGrid,
                    sizeof(bool)
                );
                file_out_stream.write(
                    (char*)&edge_iterator->isAccessRestricted,
                    sizeof(bool)
                );
                file_out_stream.write(
                    (char*)&edge_iterator->isContraFlow,
                    sizeof(bool)
                );
                ++number_of_used_edges;
            }
            ++edge_iterator;
        }
        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;
        std::cout << "[extractor] setting number of edges   ... " << std::flush;

        file_out_stream.seekp(previous_file_position);
        file_out_stream.write((char*)&number_of_used_edges, sizeof(unsigned));
        file_out_stream.close();
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
        BOOST_FOREACH(const std::string & temp_string, name_list) {
            total_number_of_chars += temp_string.length();
        }
        //write total number of chars
        name_file_stream.write(
            (char *)&(total_number_of_chars),
            sizeof(unsigned)
        );
        //write prefixe sums
        unsigned name_lengths_prefix_sum = 0;
        BOOST_FOREACH(const std::string & temp_string, name_list) {
            name_file_stream.write(
                (char *)&(name_lengths_prefix_sum),
                sizeof(unsigned)
            );
            name_lengths_prefix_sum += temp_string.length();
        }
        //duplicate on purpose!
        name_file_stream.write(
            (char *)&(name_lengths_prefix_sum),
            sizeof(unsigned)
        );

        //write all chars consecutively
        BOOST_FOREACH(const std::string & temp_string, name_list) {
            const unsigned string_length = temp_string.length();
            name_file_stream.write(temp_string.c_str(), string_length);
        }

        name_file_stream.close();
        std::cout << "ok, after " << get_timestamp() - time << "s" << std::endl;
        SimpleLogger().Write() << "Processed " <<
            number_of_used_nodes << " nodes and " <<
            number_of_used_edges << " edges";

    } catch ( const std::exception& e ) {
        std::cerr <<  "Caught Execption:" << e.what() << std::endl;
    }
}


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

#include "PBFParser.h"

#include "ExtractionWay.h"
#include "ExtractorCallbacks.h"
#include "ScriptingEnvironment.h"

#include "../DataStructures/HashTable.h"
#include "../DataStructures/ImportNode.h"
#include "../DataStructures/Restriction.h"
#include "../Util/MachineInfo.h"
#include "../Util/OpenMPWrapper.h"
#include "../Util/OSRMException.h"
#include "../Util/SimpleLogger.h"
#include "../typedefs.h"

#include <osrm/Coordinate.h>

#include <zlib.h>

#include <functional>
#include <limits>
#include <thread>

PBFParser::PBFParser(const char *fileName,
                     ExtractorCallbacks *extractor_callbacks,
                     ScriptingEnvironment &scripting_environment,
                     const bool use_elevation)
    : BaseParser(extractor_callbacks, scripting_environment, use_elevation)
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    // TODO: What is the bottleneck here? Filling the queue or reading the stuff from disk?
    // NOTE: With Lua scripting, it is parsing the stuff. I/O is virtually for free.

    // Max 2500 items in queue, hardcoded.
    thread_data_queue = std::make_shared<ConcurrentQueue<ParserThreadData *>>(2500);
    input.open(fileName, std::ios::in | std::ios::binary);

    if (!input)
    {
        throw OSRMException("pbf file not found.");
    }

    block_count = 0;
    group_count = 0;
}

PBFParser::~PBFParser()
{
    if (input.is_open())
    {
        input.close();
    }

    // Clean up any leftover ThreadData objects in the queue
    ParserThreadData *thread_data;
    while (thread_data_queue->try_pop(thread_data))
    {
        delete thread_data;
    }
    google::protobuf::ShutdownProtobufLibrary();

    SimpleLogger().Write(logDEBUG) << "parsed " << block_count << " blocks from pbf with "
                                   << group_count << " groups";
}

inline bool PBFParser::ReadHeader()
{
    ParserThreadData init_data;
    /** read Header */
    if (!readPBFBlobHeader(input, &init_data))
    {
        return false;
    }

    if (readBlob(input, &init_data))
    {
        if (!init_data.PBFHeaderBlock.ParseFromArray(&(init_data.charBuffer[0]),
                                                    init_data.charBuffer.size()))
        {
            std::cerr << "[error] Header not parseable!" << std::endl;
            return false;
        }

        const auto feature_size = init_data.PBFHeaderBlock.required_features_size();
        for (int i = 0; i < feature_size; ++i)
        {
            const std::string &feature = init_data.PBFHeaderBlock.required_features(i);
            bool supported = false;
            if ("OsmSchema-V0.6" == feature)
            {
                supported = true;
            }
            else if ("DenseNodes" == feature)
            {
                supported = true;
            }

            if (!supported)
            {
                std::cerr << "[error] required feature not supported: " << feature.data()
                          << std::endl;
                return false;
            }
        }
    }
    else
    {
        std::cerr << "[error] blob not loaded!" << std::endl;
    }
    return true;
}

inline void PBFParser::ReadData()
{
    bool keep_running = true;
    do
    {
        ParserThreadData *thread_data = new ParserThreadData();
        keep_running = readNextBlock(input, thread_data);

        if (keep_running)
        {
            thread_data_queue->push(thread_data);
        }
        else
        {
            // No more data to read, parse stops when nullptr encountered
            thread_data_queue->push(nullptr);
            delete thread_data;
        }
    } while (keep_running);
}

inline void PBFParser::ParseData()
{
    while (true)
    {
        ParserThreadData *thread_data;
        thread_data_queue->wait_and_pop(thread_data);
        if (nullptr == thread_data)
        {
            thread_data_queue->push(nullptr); // Signal end of data for other threads
            break;
        }

        loadBlock(thread_data);

        int group_size = thread_data->PBFprimitiveBlock.primitivegroup_size();
        for (int i = 0; i < group_size; ++i)
        {
            thread_data->currentGroupID = i;
            loadGroup(thread_data);

            if (thread_data->entityTypeIndicator == TypeNode)
            {
                parseNode(thread_data);
            }
            if (thread_data->entityTypeIndicator == TypeWay)
            {
                parseWay(thread_data);
            }
            if (thread_data->entityTypeIndicator == TypeRelation)
            {
                parseRelation(thread_data);
            }
            if (thread_data->entityTypeIndicator == TypeDenseNode)
            {
                parseDenseNode(thread_data);
            }
        }

        delete thread_data;
        thread_data = nullptr;
    }
}

inline bool PBFParser::Parse()
{
    // Start the read and parse threads
    std::thread read_thread(std::bind(&PBFParser::ReadData, this));

    // Open several parse threads that are synchronized before call to
    std::thread parse_thread(std::bind(&PBFParser::ParseData, this));

    // Wait for the threads to finish
    read_thread.join();
    parse_thread.join();

    return true;
}

inline void PBFParser::parseDenseNode(ParserThreadData *thread_data)
{
    const OSMPBF::DenseNodes &dense =
        thread_data->PBFprimitiveBlock.primitivegroup(thread_data->currentGroupID).dense();
    int denseTagIndex = 0;
    int64_t m_lastDenseID = 0;
    int64_t m_lastDenseLatitude = 0;
    int64_t m_lastDenseLongitude = 0;

    const int number_of_nodes = dense.id_size();
    std::vector<ImportNode> extracted_nodes_vector(number_of_nodes);
    for (int i = 0; i < number_of_nodes; ++i)
    {
        m_lastDenseID += dense.id(i);
        m_lastDenseLatitude += dense.lat(i);
        m_lastDenseLongitude += dense.lon(i);
        extracted_nodes_vector[i].id = m_lastDenseID;
        extracted_nodes_vector[i].lat =
            COORDINATE_PRECISION *
            ((double)m_lastDenseLatitude * thread_data->PBFprimitiveBlock.granularity() +
             thread_data->PBFprimitiveBlock.lat_offset()) /
            NANO;
        extracted_nodes_vector[i].lon =
            COORDINATE_PRECISION *
            ((double)m_lastDenseLongitude * thread_data->PBFprimitiveBlock.granularity() +
             thread_data->PBFprimitiveBlock.lon_offset()) /
            NANO;
        while (denseTagIndex < dense.keys_vals_size())
        {
            const int tagValue = dense.keys_vals(denseTagIndex);
            if (0 == tagValue)
            {
                ++denseTagIndex;
                break;
            }
            const int keyValue = dense.keys_vals(denseTagIndex + 1);
            const std::string &key = thread_data->PBFprimitiveBlock.stringtable().s(tagValue);
            const std::string &value = thread_data->PBFprimitiveBlock.stringtable().s(keyValue);
            extracted_nodes_vector[i].keyVals.emplace(key, value);
            denseTagIndex += 2;
        }
    }
#pragma omp parallel
    {
        const int thread_num = omp_get_thread_num();
#pragma omp parallel for schedule(guided)
        for (int i = 0; i < number_of_nodes; ++i)
        {
            ImportNode &import_node = extracted_nodes_vector[i];
            ParseNodeInLua(import_node, scripting_environment.getLuaStateForThreadID(thread_num));
        }
    }

    for (const ImportNode &import_node : extracted_nodes_vector)
    {
        extractor_callbacks->ProcessNode(import_node, use_elevation);
    }
}

inline void PBFParser::parseNode(ParserThreadData *)
{
    throw OSRMException("Parsing of simple nodes not supported. PBF should use dense nodes");
}

inline void PBFParser::parseRelation(ParserThreadData *thread_data)
{
    // TODO: leave early, if relation is not a restriction
    // TODO: reuse rawRestriction container
    if (!use_turn_restrictions)
    {
        return;
    }
    const OSMPBF::PrimitiveGroup &group =
        thread_data->PBFprimitiveBlock.primitivegroup(thread_data->currentGroupID);

    for (int i = 0, relation_size = group.relations_size(); i < relation_size; ++i)
    {
        std::string except_tag_string;
        const OSMPBF::Relation &inputRelation =
            thread_data->PBFprimitiveBlock.primitivegroup(thread_data->currentGroupID).relations(i);
        bool isRestriction = false;
        bool isOnlyRestriction = false;
        for (int k = 0, endOfKeys = inputRelation.keys_size(); k < endOfKeys; ++k)
        {
            const std::string &key =
                thread_data->PBFprimitiveBlock.stringtable().s(inputRelation.keys(k));
            const std::string &val =
                thread_data->PBFprimitiveBlock.stringtable().s(inputRelation.vals(k));
            if ("type" == key)
            {
                if ("restriction" == val)
                {
                    isRestriction = true;
                }
                else
                {
                    break;
                }
            }
            if (("restriction" == key) && (val.find("only_") == 0))
            {
                isOnlyRestriction = true;
            }
            if ("except" == key)
            {
                except_tag_string = val;
            }
        }

        if (isRestriction && ShouldIgnoreRestriction(except_tag_string))
        {
            continue;
        }

        if (isRestriction)
        {
            int64_t lastRef = 0;
            InputRestrictionContainer currentRestrictionContainer(isOnlyRestriction);
            for (int rolesIndex = 0, last_role = inputRelation.roles_sid_size();
                 rolesIndex < last_role;
                 ++rolesIndex)
            {
                const std::string &role = thread_data->PBFprimitiveBlock.stringtable().s(
                    inputRelation.roles_sid(rolesIndex));
                lastRef += inputRelation.memids(rolesIndex);

                if (!("from" == role || "to" == role || "via" == role))
                {
                    continue;
                }

                switch (inputRelation.types(rolesIndex))
                {
                case 0: // node
                    if ("from" == role || "to" == role)
                    { // Only via should be a node
                        continue;
                    }
                    assert("via" == role);
                    if (std::numeric_limits<unsigned>::max() != currentRestrictionContainer.viaNode)
                    {
                        currentRestrictionContainer.viaNode = std::numeric_limits<unsigned>::max();
                    }
                    assert(std::numeric_limits<unsigned>::max() == currentRestrictionContainer.viaNode);
                    currentRestrictionContainer.restriction.viaNode = lastRef;
                    break;
                case 1: // way
                    assert("from" == role || "to" == role || "via" == role);
                    if ("from" == role)
                    {
                        currentRestrictionContainer.fromWay = lastRef;
                    }
                    if ("to" == role)
                    {
                        currentRestrictionContainer.toWay = lastRef;
                    }
                    if ("via" == role)
                    {
                        assert(currentRestrictionContainer.restriction.toNode == std::numeric_limits<unsigned>::max());
                        currentRestrictionContainer.viaNode = lastRef;
                    }
                    break;
                case 2: // relation, not used. relations relating to relations are evil.
                    continue;
                    assert(false);
                    break;

                default: // should not happen
                    assert(false);
                    break;
                }
            }
            if (!extractor_callbacks->ProcessRestriction(currentRestrictionContainer))
            {
                std::cerr << "[PBFParser] relation not parsed" << std::endl;
            }
        }
    }
}

inline void PBFParser::parseWay(ParserThreadData *thread_data)
{
    const int number_of_ways =
        thread_data->PBFprimitiveBlock.primitivegroup(thread_data->currentGroupID).ways_size();
    std::vector<ExtractionWay> parsed_way_vector(number_of_ways);
    for (int i = 0; i < number_of_ways; ++i)
    {
        const OSMPBF::Way &inputWay =
            thread_data->PBFprimitiveBlock.primitivegroup(thread_data->currentGroupID).ways(i);
        parsed_way_vector[i].id = inputWay.id();
        unsigned pathNode(0);
        const int number_of_referenced_nodes = inputWay.refs_size();
        for (int j = 0; j < number_of_referenced_nodes; ++j)
        {
            pathNode += inputWay.refs(j);
            parsed_way_vector[i].path.push_back(pathNode);
        }
        assert(inputWay.keys_size() == inputWay.vals_size());
        const int number_of_keys = inputWay.keys_size();
        for (int j = 0; j < number_of_keys; ++j)
        {
            const std::string &key =
                thread_data->PBFprimitiveBlock.stringtable().s(inputWay.keys(j));
            const std::string &val =
                thread_data->PBFprimitiveBlock.stringtable().s(inputWay.vals(j));
            parsed_way_vector[i].keyVals.emplace(key, val);
        }
    }

#pragma omp parallel for schedule(guided)
    for (int i = 0; i < number_of_ways; ++i)
    {
        ExtractionWay &extraction_way = parsed_way_vector[i];
        if (2 <= extraction_way.path.size())
        {
            ParseWayInLua(extraction_way,
                          scripting_environment.getLuaStateForThreadID(omp_get_thread_num()));
        }
    }

    for (ExtractionWay &extraction_way : parsed_way_vector)
    {
        if (2 <= extraction_way.path.size())
        {
            extractor_callbacks->ProcessWay(extraction_way);
        }
    }
}

inline void PBFParser::loadGroup(ParserThreadData *thread_data)
{
#ifndef NDEBUG
    ++group_count;
#endif

    const OSMPBF::PrimitiveGroup &group =
        thread_data->PBFprimitiveBlock.primitivegroup(thread_data->currentGroupID);
    thread_data->entityTypeIndicator = TypeDummy;
    if (0 != group.nodes_size())
    {
        thread_data->entityTypeIndicator = TypeNode;
    }
    if (0 != group.ways_size())
    {
        thread_data->entityTypeIndicator = TypeWay;
    }
    if (0 != group.relations_size())
    {
        thread_data->entityTypeIndicator = TypeRelation;
    }
    if (group.has_dense())
    {
        thread_data->entityTypeIndicator = TypeDenseNode;
        assert(0 != group.dense().id_size());
    }
    assert(thread_data->entityTypeIndicator != TypeDummy);
}

inline void PBFParser::loadBlock(ParserThreadData *thread_data)
{
    ++block_count;
    thread_data->currentGroupID = 0;
    thread_data->currentEntityID = 0;
}

inline bool PBFParser::readPBFBlobHeader(std::fstream &stream, ParserThreadData *thread_data)
{
    int size(0);
    stream.read((char *)&size, sizeof(int));
    size = SwapEndian(size);
    if (stream.eof())
    {
        return false;
    }
    if (size > MAX_BLOB_HEADER_SIZE || size < 0)
    {
        return false;
    }
    char *data = new char[size];
    stream.read(data, size * sizeof(data[0]));

    bool dataSuccessfullyParsed = (thread_data->PBFBlobHeader).ParseFromArray(data, size);
    delete[] data;
    return dataSuccessfullyParsed;
}

inline bool PBFParser::unpackZLIB(std::fstream &, ParserThreadData *thread_data)
{
    unsigned rawSize = thread_data->PBFBlob.raw_size();
    char *unpackedDataArray = new char[rawSize];
    z_stream compressedDataStream;
    compressedDataStream.next_in = (unsigned char *)thread_data->PBFBlob.zlib_data().data();
    compressedDataStream.avail_in = thread_data->PBFBlob.zlib_data().size();
    compressedDataStream.next_out = (unsigned char *)unpackedDataArray;
    compressedDataStream.avail_out = rawSize;
    compressedDataStream.zalloc = Z_NULL;
    compressedDataStream.zfree = Z_NULL;
    compressedDataStream.opaque = Z_NULL;
    int ret = inflateInit(&compressedDataStream);
    if (ret != Z_OK)
    {
        std::cerr << "[error] failed to init zlib stream" << std::endl;
        delete[] unpackedDataArray;
        return false;
    }

    ret = inflate(&compressedDataStream, Z_FINISH);
    if (ret != Z_STREAM_END)
    {
        std::cerr << "[error] failed to inflate zlib stream" << std::endl;
        std::cerr << "[error] Error type: " << ret << std::endl;
        delete[] unpackedDataArray;
        return false;
    }

    ret = inflateEnd(&compressedDataStream);
    if (ret != Z_OK)
    {
        std::cerr << "[error] failed to deinit zlib stream" << std::endl;
        delete[] unpackedDataArray;
        return false;
    }

    thread_data->charBuffer.clear();
    thread_data->charBuffer.resize(rawSize);
    std::copy(unpackedDataArray, unpackedDataArray + rawSize, thread_data->charBuffer.begin());
    delete[] unpackedDataArray;
    return true;
}

inline bool PBFParser::unpackLZMA(std::fstream &, ParserThreadData *) { return false; }

inline bool PBFParser::readBlob(std::fstream &stream, ParserThreadData *thread_data)
{
    if (stream.eof())
    {
        return false;
    }

    const int size = thread_data->PBFBlobHeader.datasize();
    if (size < 0 || size > MAX_BLOB_SIZE)
    {
        std::cerr << "[error] invalid Blob size:" << size << std::endl;
        return false;
    }

    char *data = new char[size];
    stream.read(data, sizeof(data[0]) * size);

    if (!thread_data->PBFBlob.ParseFromArray(data, size))
    {
        std::cerr << "[error] failed to parse blob" << std::endl;
        delete[] data;
        return false;
    }

    if (thread_data->PBFBlob.has_raw())
    {
        const std::string &data = thread_data->PBFBlob.raw();
        thread_data->charBuffer.clear();
        thread_data->charBuffer.resize(data.size());
        std::copy(data.begin(), data.end(), thread_data->charBuffer.begin());
    }
    else if (thread_data->PBFBlob.has_zlib_data())
    {
        if (!unpackZLIB(stream, thread_data))
        {
            std::cerr << "[error] zlib data encountered that could not be unpacked" << std::endl;
            delete[] data;
            return false;
        }
    }
    else if (thread_data->PBFBlob.has_lzma_data())
    {
        if (!unpackLZMA(stream, thread_data))
        {
            std::cerr << "[error] lzma data encountered that could not be unpacked" << std::endl;
        }
        delete[] data;
        return false;
    }
    else
    {
        std::cerr << "[error] Blob contains no data" << std::endl;
        delete[] data;
        return false;
    }
    delete[] data;
    return true;
}

bool PBFParser::readNextBlock(std::fstream &stream, ParserThreadData *thread_data)
{
    if (stream.eof())
    {
        return false;
    }

    if (!readPBFBlobHeader(stream, thread_data))
    {
        return false;
    }

    if (thread_data->PBFBlobHeader.type() != "OSMData")
    {
        return false;
    }

    if (!readBlob(stream, thread_data))
    {
        return false;
    }

    if (!thread_data->PBFprimitiveBlock.ParseFromArray(&(thread_data->charBuffer[0]),
                                                      thread_data->charBuffer.size()))
    {
        std::cerr << "failed to parse PrimitiveBlock" << std::endl;
        return false;
    }
    return true;
}

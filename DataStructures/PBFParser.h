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

#ifndef PBFPARSER_H_
#define PBFPARSER_H_

#include <zlib.h>

#include "BaseParser.h"

#include "pbf-proto/fileformat.pb.h"
#include "pbf-proto/osmformat.pb.h"
#include "../typedefs.h"
#include "HashTable.h"
#include "ExtractorStructs.h"


class PBFParser : public BaseParser<_Node, _Relation, _Way> {

    enum EntityType {
        TypeNode = 1,
        TypeWay = 2,
        TypeRelation = 4,
        TypeDenseNode = 8
    } ;

    enum Endianness {
        LittleEndian = 1,
        BigEndian = 2
    };

    struct _ThreadData {
        int currentGroupID;
        int currentEntityID;
        short entityTypeIndicator;

        OSMPBF::BlobHeader PBFBlobHeader;
        OSMPBF::Blob PBFBlob;

        OSMPBF::HeaderBlock PBFHeaderBlock;
        OSMPBF::PrimitiveBlock PBFprimitiveBlock;

        std::vector<char> charBuffer;
    };

public:
    PBFParser(const char * fileName) {
        GOOGLE_PROTOBUF_VERIFY_VERSION;
        //        omp_set_num_threads(1);
        input.open(fileName, std::ios::in | std::ios::binary);

        if (!input) {
            std::cerr << fileName << ": File not found." << std::endl;
        }

        blockCount = 0;
        groupCount = 0;
    }

    bool RegisterCallbacks(bool (*nodeCallbackPointer)(_Node), bool (*relationCallbackPointer)(_Relation), bool (*wayCallbackPointer)(_Way),bool (*addressCallbackPointer)(_Node, HashTable<std::string, std::string>) ) {
        nodeCallback = *nodeCallbackPointer;
        wayCallback = *wayCallbackPointer;
        relationCallback = *relationCallbackPointer;
        addressCallback = *addressCallbackPointer;
        return true;
    }

    ~PBFParser() {
        if(input.is_open())
            input.close();

        unsigned maxThreads = omp_get_max_threads();
        for ( unsigned threadNum = 0; threadNum < maxThreads; ++threadNum ) {
            delete threadDataVector[threadNum];
        }

        google::protobuf::ShutdownProtobufLibrary();

        std::cout << "[info] blocks: " << blockCount << std::endl;
        std::cout << "[info] groups: " << groupCount << std::endl;
    }

    bool Init() {
        /** Init Vector with ThreadData Objects */
        unsigned maxThreads = omp_get_max_threads();
        for ( unsigned threadNum = 0; threadNum < maxThreads; ++threadNum ) {
            threadDataVector.push_back( new _ThreadData( ) );
        }

        _ThreadData initData;
        /** read Header */
        if(!readPBFBlobHeader(input, &initData)) {
            return false;
        }

        if(readBlob(input, &initData)) {
            char data[initData.charBuffer.size()];
            for(unsigned i = 0; i < initData.charBuffer.size(); i++ ){
                data[i] = initData.charBuffer[i];
            }
            if(!initData.PBFHeaderBlock.ParseFromArray(data, initData.charBuffer.size() ) ) {
                std::cerr << "[error] Header not parseable!" << std::endl;
                return false;
            }

            for(int i = 0; i < initData.PBFHeaderBlock.required_features_size(); i++) {
                const std::string& feature = initData.PBFHeaderBlock.required_features( i );
                bool supported = false;
                if ( feature == "OsmSchema-V0.6" )
                    supported = true;
                else if ( feature == "DenseNodes" )
                    supported = true;

                if ( !supported ) {
                    std::cerr << "[error] required feature not supported: " << feature.data() << std::endl;
                    return false;
                }
            }
        } else {
            std::cerr << "[error] blob not loaded!" << std::endl;
        }
        return true;
    }

    bool Parse() {
#pragma omp parallel
        {
            _ThreadData * threadData = threadDataVector[omp_get_thread_num()];
            //parse through all Blocks
            bool keepRunning = true;
            //        while(readNextBlock(input)) {
            do{
#pragma omp critical
                {
                    keepRunning = readNextBlock(input, threadData);
                }
                if(keepRunning) {
                    loadBlock(threadData);
                    for(int i = 0; i < threadData->PBFprimitiveBlock.primitivegroup_size(); i++) {
                        threadData->currentGroupID = i;
                        loadGroup(threadData);

                        if(threadData->entityTypeIndicator == TypeNode)
                            parseNode(threadData);
                        if(threadData->entityTypeIndicator == TypeWay)
                            parseWay(threadData);
                        if(threadData->entityTypeIndicator == TypeRelation)
                            parseRelation(threadData);
                        if(threadData->entityTypeIndicator == TypeDenseNode)
                            parseDenseNode(threadData);
                    }
                }
            }while(keepRunning);
        }
        return true;
    }

private:

    void parseDenseNode(_ThreadData * threadData) {
        const OSMPBF::DenseNodes& dense = threadData->PBFprimitiveBlock.primitivegroup( threadData->currentGroupID ).dense();
        int denseTagIndex = 0;
        int m_lastDenseID = 0;
        int m_lastDenseLatitude = 0;
        int m_lastDenseLongitude = 0;

        for(int i = 0; i < dense.id_size(); i++) {
            HashTable<std::string, std::string> keyVals;
            m_lastDenseID += dense.id( i );
            m_lastDenseLatitude += dense.lat( i );
            m_lastDenseLongitude += dense.lon( i );
            _Node n;
            n.id = m_lastDenseID;
            n.lat = 100000*( ( double ) m_lastDenseLatitude * threadData->PBFprimitiveBlock.granularity() +threadData-> PBFprimitiveBlock.lat_offset() ) / NANO;
            n.lon = 100000*( ( double ) m_lastDenseLongitude * threadData->PBFprimitiveBlock.granularity() + threadData->PBFprimitiveBlock.lon_offset() ) / NANO;
            while (denseTagIndex < dense.keys_vals_size()) {
                int tagValue = dense.keys_vals( denseTagIndex );
                if(tagValue == 0) {
                    denseTagIndex++;
                    break;
                }
                int keyValue = dense.keys_vals ( denseTagIndex+1 );
                std::string key = threadData->PBFprimitiveBlock.stringtable().s(tagValue).data();
                std::string value = threadData->PBFprimitiveBlock.stringtable().s(keyValue).data();
                keyVals.Add(key, value);
                denseTagIndex += 2;
            }
#pragma omp critical
            {
                if(!(*addressCallback)(n, keyVals))
                    std::cerr << "[PBFParser] adress not parsed" << std::endl;
            }

#pragma omp critical
            {
                if(!(*nodeCallback)(n))
                    std::cerr << "[PBFParser] dense node not parsed" << std::endl;
            }
        }
    }

    void parseNode(_ThreadData * threadData) {
        _Node n;
#pragma omp critical
        {
            if(!(*nodeCallback)(n))
                std::cerr << "[PBFParser] simple node not parsed" << std::endl;
        }
    }

    void parseRelation(_ThreadData * threadData) {
        const OSMPBF::PrimitiveGroup& group = threadData->PBFprimitiveBlock.primitivegroup( threadData->currentGroupID );
        for(int i = 0; i < group.relations_size(); i++ ) {
            _Relation r;
            r.type = _Relation::unknown;
#pragma omp critical
            {
                if(!(*relationCallback)(r))
                    std::cerr << "[PBFParser] relation not parsed" << std::endl;
            }
        }
    }

    void parseWay(_ThreadData * threadData) {
        if( threadData->PBFprimitiveBlock.primitivegroup( threadData->currentGroupID ).ways_size() > 0) {
            for(int i = 0; i < threadData->PBFprimitiveBlock.primitivegroup( threadData->currentGroupID ).ways_size(); i++) {
                const OSMPBF::Way& inputWay = threadData->PBFprimitiveBlock.primitivegroup( threadData->currentGroupID ).ways( i );
                _Way w;
                w.id = inputWay.id();
                unsigned pathNode(0);
                for(int i = 0; i < inputWay.refs_size(); i++) {
                    pathNode += inputWay.refs(i);
                    w.path.push_back(pathNode);
                }
                assert(inputWay.keys_size() == inputWay.vals_size());
                for(int i = 0; i < inputWay.keys_size(); i++) {
                    const std::string key = threadData->PBFprimitiveBlock.stringtable().s(inputWay.keys(i));
                    const std::string val = threadData->PBFprimitiveBlock.stringtable().s(inputWay.vals(i));
                    w.keyVals.Add(key, val);
                }
#pragma omp critical
                {
                    if(!(*wayCallback)(w)) {
                        std::cerr << "[PBFParser] way not parsed" << std::endl;
                    }
                }
            }
        }
    }

    void loadGroup(_ThreadData * threadData) {
#pragma omp atomic
        groupCount++;

        const OSMPBF::PrimitiveGroup& group = threadData->PBFprimitiveBlock.primitivegroup( threadData->currentGroupID );
        threadData->entityTypeIndicator = 0;
        if ( group.nodes_size() != 0 ) {
            threadData->entityTypeIndicator = TypeNode;
        }
        if ( group.ways_size() != 0 ) {
            threadData->entityTypeIndicator = TypeWay;
        }
        if ( group.relations_size() != 0 ) {
            threadData->entityTypeIndicator = TypeRelation;
        }
        if ( group.has_dense() )  {
            threadData->entityTypeIndicator = TypeDenseNode;
            assert( group.dense().id_size() != 0 );
        }
        assert( threadData->entityTypeIndicator != 0 );
    }

    void loadBlock(_ThreadData * threadData) {
#pragma omp critical
        blockCount++;
        threadData->currentGroupID = 0;
        threadData->currentEntityID = 0;
    }

    /* Reverses Network Byte Order into something usable */
    inline unsigned swapEndian(unsigned x) {
        if(getMachineEndianness() == LittleEndian)
            return ( (x>>24) | ((x<<8) & 0x00FF0000) | ((x>>8) & 0x0000FF00) | (x<<24) );
        return x;
    }

    bool readPBFBlobHeader(std::fstream& stream, _ThreadData * threadData) {
        int size(0);
        stream.read((char *)&size, sizeof(int));
        size = swapEndian(size);
        if(stream.eof()) {
            return false;
        }
        if ( size > MAX_BLOB_HEADER_SIZE || size < 0 ) {
            return false;
        }
        char data[size];
        for(int i = 0; i < size; i++) {
            stream.read(&data[i], 1);
        }

        if ( !(threadData->PBFBlobHeader).ParseFromArray( data, size ) ){
            return false;
        }
        return true;
    }

    bool unpackZLIB(std::fstream & stream, _ThreadData * threadData) {
        unsigned rawSize = threadData->PBFBlob.raw_size();
        char unpackedDataArray[rawSize];
        z_stream compressedDataStream;
        compressedDataStream.next_in = ( unsigned char* ) threadData->PBFBlob.zlib_data().data();
        compressedDataStream.avail_in = threadData->PBFBlob.zlib_data().size();
        compressedDataStream.next_out = ( unsigned char* ) unpackedDataArray;
        compressedDataStream.avail_out = rawSize;
        compressedDataStream.zalloc = Z_NULL;
        compressedDataStream.zfree = Z_NULL;
        compressedDataStream.opaque = Z_NULL;
        int ret = inflateInit( &compressedDataStream );
        if ( ret != Z_OK ) {
            std::cerr << "[error] failed to init zlib stream" << std::endl;
            return false;
        }

        ret = inflate( &compressedDataStream, Z_FINISH );
        if ( ret != Z_STREAM_END ) {
            std::cerr << "[error] failed to inflate zlib stream" << std::endl;
            std::cerr << "[error] Error type: " << ret << std::endl;
            return false;
        }

        ret = inflateEnd( &compressedDataStream );
        if ( ret != Z_OK ) {
            std::cerr << "[error] failed to deinit zlib stream" << std::endl;
            return false;
        }

        threadData->charBuffer.clear(); threadData->charBuffer.resize(rawSize);
        for(unsigned i = 0; i < rawSize; i++) {
            threadData->charBuffer[i] = unpackedDataArray[i];
        }
        return true;
    }

    bool unpackLZMA(std::fstream & stream, _ThreadData * threadData) {
        return false;
    }

    bool readBlob(std::fstream& stream, _ThreadData * threadData) {
        if(stream.eof())
            return false;

        int size = threadData->PBFBlobHeader.datasize();
        if ( size < 0 || size > MAX_BLOB_SIZE ) {
            std::cerr << "[error] invalid Blob size:" << size << std::endl;
            return false;
        }

        char data[size];
        for(int i = 0; i < size; i++) {
            stream.read(&data[i], 1);
        }

        if ( !threadData->PBFBlob.ParseFromArray( data, size ) ) {
            std::cerr << "[error] failed to parse blob" << std::endl;
            return false;
        }

        if ( threadData->PBFBlob.has_raw() ) {
            const std::string& data = threadData->PBFBlob.raw();
            threadData->charBuffer.clear();
            threadData->charBuffer.resize( data.size() );
            for ( unsigned i = 0; i < data.size(); i++ ) {
                threadData->charBuffer[i] = data[i];
            }
        } else if ( threadData->PBFBlob.has_zlib_data() ) {
            if ( !unpackZLIB(stream, threadData) ) {
                std::cerr << "[error] zlib data encountered that could not be unpacked" << std::endl;
                return false;
            }
        } else if ( threadData->PBFBlob.has_lzma_data() ) {
            if ( !unpackLZMA(stream, threadData) )
                std::cerr << "[error] lzma data encountered that could not be unpacked" << std::endl;
            return false;
        } else {
            std::cerr << "[error] Blob contains no data" << std::endl;
            return false;
        }
        return true;
    }

    bool readNextBlock(std::fstream& stream, _ThreadData * threadData) {
        if(stream.eof()) {
            return false;
        }

        if ( !readPBFBlobHeader(stream, threadData) )
            return false;

        if ( threadData->PBFBlobHeader.type() != "OSMData" ) {
            std::cerr << "[error] invalid block type, found" << threadData->PBFBlobHeader.type().data() << "instead of OSMData" << std::endl;
            return false;
        }

        if ( !readBlob(stream, threadData) )
            return false;

        char data[threadData->charBuffer.size()];
        for(unsigned i = 0; i < threadData->charBuffer.size(); i++ ){
            data[i] = threadData->charBuffer[i];
        }
        if ( !threadData->PBFprimitiveBlock.ParseFromArray( data, threadData-> charBuffer.size() ) ) {
            std::cerr << "[error] failed to parse PrimitiveBlock" << std::endl;
            return false;
        }
        return true;
    }

    static Endianness getMachineEndianness() {
        int i(1);
        char *p = (char *) &i;
        if (p[0] == 1)
            return LittleEndian;
        return BigEndian;
    }

    static const int NANO = 1000 * 1000 * 1000;
    static const int MAX_BLOB_HEADER_SIZE = 64 * 1024;
    static const int MAX_BLOB_SIZE = 32 * 1024 * 1024;

    /* counting the number of read blocks and groups */
    unsigned groupCount;
    unsigned blockCount;

    /* Function pointer for nodes */
    bool (*nodeCallback)(_Node);
    bool (*wayCallback)(_Way);
    bool (*relationCallback)(_Relation);
    bool (*addressCallback)(_Node, HashTable<std::string, std::string>);
    /* the input stream to parse */
    std::fstream input;

    /* ThreadData Array */
    std::vector < _ThreadData* > threadDataVector;

};

#endif /* PBFPARSER_H_ */

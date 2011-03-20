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

    short entityTypeIndicator;

public:
    PBFParser(const char * fileName) {
        GOOGLE_PROTOBUF_VERIFY_VERSION;

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

        google::protobuf::ShutdownProtobufLibrary();

        std::cout << "[info] blocks: " << blockCount << std::endl;
        std::cout << "[info] groups: " << groupCount << std::endl;
    }

    bool Init() {
        if(!readPBFBlobHeader(input)) {
            return false;
        }

        if(readBlob(input)) {
            char data[charBuffer.size()];
            for(unsigned i = 0; i < charBuffer.size(); i++ ){
                data[i] = charBuffer[i];
            }
            if(!PBFHeaderBlock.ParseFromArray(data, charBuffer.size() ) ) {
                std::cerr << "[error] Header not parseable!" << std::endl;
                return false;
            }

            for(int i = 0; i < PBFHeaderBlock.required_features_size(); i++) {
                const std::string& feature = PBFHeaderBlock.required_features( i );
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
        //parse through all Blocks
        while(readNextBlock(input)) {

            loadBlock();
            for(int i = 0; i < PBFprimitiveBlock.primitivegroup_size(); i++) {
                currentGroupID = i;
                loadGroup();

                if(entityTypeIndicator == TypeNode)
                    parseNode();
                if(entityTypeIndicator == TypeWay)
                    parseWay();
                if(entityTypeIndicator == TypeRelation)
                    parseRelation();
                if(entityTypeIndicator == TypeDenseNode)
                    parseDenseNode();
            }
        }
        return true;
    }

private:

    void parseDenseNode() {
        const OSMPBF::DenseNodes& dense = PBFprimitiveBlock.primitivegroup( currentGroupID ).dense();
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
            n.lat = 100000*( ( double ) m_lastDenseLatitude * PBFprimitiveBlock.granularity() + PBFprimitiveBlock.lat_offset() ) / NANO;
            n.lon = 100000*( ( double ) m_lastDenseLongitude * PBFprimitiveBlock.granularity() + PBFprimitiveBlock.lon_offset() ) / NANO;
            while (denseTagIndex < dense.keys_vals_size()) {
                int tagValue = dense.keys_vals( denseTagIndex );
                if(tagValue == 0) {
                    denseTagIndex++;
                    break;
                }
                int keyValue = dense.keys_vals ( denseTagIndex+1 );
                std::string key = PBFprimitiveBlock.stringtable().s(tagValue).data();
                std::string value = PBFprimitiveBlock.stringtable().s(keyValue).data();
                keyVals.Add(key, value);
                denseTagIndex += 2;
            }
            if(!(*addressCallback)(n, keyVals))
                std::cerr << "[PBFParser] adress not parsed" << std::endl;

            if(!(*nodeCallback)(n))
                std::cerr << "[PBFParser] dense node not parsed" << std::endl;
        }
    }

    void parseNode() {
        _Node n;
        if(!(*nodeCallback)(n))
            std::cerr << "[PBFParser] simple node not parsed" << std::endl;
    }

    void parseRelation() {
        const OSMPBF::PrimitiveGroup& group = PBFprimitiveBlock.primitivegroup( currentGroupID );
        for(int i = 0; i < group.relations_size(); i++ ) {
            _Relation r;
            r.type = _Relation::unknown;
            if(!(*relationCallback)(r))
                std::cerr << "[PBFParser] relation not parsed" << std::endl;
        }
    }

    void parseWay() {
        if( PBFprimitiveBlock.primitivegroup( currentGroupID ).ways_size() > 0) {
            for(int i = 0; i < PBFprimitiveBlock.primitivegroup( currentGroupID ).ways_size(); i++) {
                const OSMPBF::Way& inputWay = PBFprimitiveBlock.primitivegroup( currentGroupID ).ways( i );
                _Way w;
                w.id = inputWay.id();
                unsigned pathNode(0);
                for(int i = 0; i < inputWay.refs_size(); i++) {
                    pathNode += inputWay.refs(i);
                    w.path.push_back(pathNode);
                }
                assert(inputWay.keys_size() == inputWay.vals_size());
                for(int i = 0; i < inputWay.keys_size(); i++) {
                    const std::string key = PBFprimitiveBlock.stringtable().s(inputWay.keys(i));
                    const std::string val = PBFprimitiveBlock.stringtable().s(inputWay.vals(i));
                    w.keyVals.Add(key, val);
                }
                if(!(*wayCallback)(w)) {
                    std::cerr << "[PBFParser] way not parsed" << std::endl;
                }
            }
        }
    }

    void loadGroup() {
        groupCount++;
        const OSMPBF::PrimitiveGroup& group = PBFprimitiveBlock.primitivegroup( currentGroupID );
        entityTypeIndicator = 0;
        if ( group.nodes_size() != 0 ) {
            entityTypeIndicator = TypeNode;
        }
        if ( group.ways_size() != 0 ) {
            entityTypeIndicator = TypeWay;
        }
        if ( group.relations_size() != 0 ) {
            entityTypeIndicator = TypeRelation;
        }
        if ( group.has_dense() )  {
            entityTypeIndicator = TypeDenseNode;
            assert( group.dense().id_size() != 0 );
        }
        assert( entityTypeIndicator != 0 );
    }

    void loadBlock() {
        blockCount++;
        currentGroupID = 0;
        currentEntityID = 0;
    }

    /* Reverses Network Byte Order into something usable */
    inline unsigned swapEndian(unsigned x) {
        if(getMachineEndianness() == LittleEndian)
            return ( (x>>24) | ((x<<8) & 0x00FF0000) | ((x>>8) & 0x0000FF00) | (x<<24) );
        return x;
    }

    bool readPBFBlobHeader(std::fstream& stream) {
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

        if ( !PBFBlobHeader.ParseFromArray( data, size ) ) {
            return false;
        }
        return true;
    }

    bool unpackZLIB(std::fstream & stream) {
        unsigned rawSize = PBFBlob.raw_size();
        char unpackedDataArray[rawSize];
        z_stream compressedDataStream;
        compressedDataStream.next_in = ( unsigned char* ) PBFBlob.zlib_data().data();
        compressedDataStream.avail_in = PBFBlob.zlib_data().size();
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

        charBuffer.clear(); charBuffer.resize(rawSize);
        for(unsigned i = 0; i < rawSize; i++) {
            charBuffer[i] = unpackedDataArray[i];
        }
        return true;
    }

    bool unpackLZMA(std::fstream & stream) {
        return false;
    }

    bool readBlob(std::fstream& stream) {
        if(stream.eof())
            return false;

        int size = PBFBlobHeader.datasize();
        if ( size < 0 || size > MAX_BLOB_SIZE ) {
            std::cerr << "[error] invalid Blob size:" << size << std::endl;
            return false;
        }

        char data[size];
        for(int i = 0; i < size; i++) {
            stream.read(&data[i], 1);
        }

        if ( !PBFBlob.ParseFromArray( data, size ) ) {
            std::cerr << "[error] failed to parse blob" << std::endl;
            return false;
        }

        if ( PBFBlob.has_raw() ) {
            const std::string& data = PBFBlob.raw();
            charBuffer.clear();
            charBuffer.resize( data.size() );
            for ( unsigned i = 0; i < data.size(); i++ ) {
                charBuffer[i] = data[i];
            }
        } else if ( PBFBlob.has_zlib_data() ) {
            if ( !unpackZLIB(stream) ) {
                std::cerr << "[error] zlib data encountered that could not be unpacked" << std::endl;
                return false;
            }
        } else if ( PBFBlob.has_lzma_data() ) {
            if ( !unpackLZMA(stream) )
                std::cerr << "[error] lzma data encountered that could not be unpacked" << std::endl;
            return false;
        } else {
            std::cerr << "[error] Blob contains no data" << std::endl;
            return false;
        }
        return true;
    }

    bool readNextBlock(std::fstream& stream) {
        if(stream.eof()) {
            return false;
        }

        if ( !readPBFBlobHeader(stream) )
            return false;

        if ( PBFBlobHeader.type() != "OSMData" ) {
            std::cerr << "[error] invalid block type, found" << PBFBlobHeader.type().data() << "instead of OSMData" << std::endl;
            return false;
        }

        if ( !readBlob(stream) )
            return false;

        char data[charBuffer.size()];
        for(unsigned i = 0; i < charBuffer.size(); i++ ){
            data[i] = charBuffer[i];
        }
        if ( !PBFprimitiveBlock.ParseFromArray( data, charBuffer.size() ) ) {
            std::cerr << "[error] failed to parse PrimitiveBlock" << std::endl;
            return false;
        }
        return true;
    }

    Endianness getMachineEndianness() {
        int i(1);
        char *p = (char *) &i;
        if (p[0] == 1)
            return LittleEndian;
        return BigEndian;
    }

    static const int NANO = 1000 * 1000 * 1000;
    static const int MAX_BLOB_HEADER_SIZE = 64 * 1024;
    static const int MAX_BLOB_SIZE = 32 * 1024 * 1024;

    OSMPBF::BlobHeader PBFBlobHeader;
    OSMPBF::Blob PBFBlob;

    OSMPBF::HeaderBlock PBFHeaderBlock;
    OSMPBF::PrimitiveBlock PBFprimitiveBlock;

    std::vector<char> charBuffer;

    int currentGroupID;
    int currentEntityID;

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
};

#endif /* PBFPARSER_H_ */

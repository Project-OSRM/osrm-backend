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
#include <boost/make_shared.hpp>
#include <boost/ref.hpp>
#include <boost/shared_ptr.hpp>

#include <osmpbf/fileformat.pb.h>
#include <osmpbf/osmformat.pb.h>

#include "../typedefs.h"

#include "BaseParser.h"
#include "ExtractorCallbacks.h"
#include "ExtractorStructs.h"
#include "../DataStructures/HashTable.h"
#include "../DataStructures/ConcurrentQueue.h"

class PBFParser : public BaseParser<ExtractorCallbacks, _Node, _RawRestrictionContainer, _Way> {

//    typedef BaseParser<ExtractorCallbacks, _Node, _RawRestrictionContainer, _Way> super;

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
    PBFParser(const char * fileName) : externalMemory(NULL), myLuaState(NULL) {
        GOOGLE_PROTOBUF_VERIFY_VERSION;
        //TODO: What is the bottleneck here? Filling the queue or reading the stuff from disk?
        threadDataQueue = boost::make_shared<ConcurrentQueue<_ThreadData*> >( 2500 ); /* Max 2500 items in queue, hardcoded. */
        input.open(fileName, std::ios::in | std::ios::binary);

        if (!input) {
            std::cerr << fileName << ": File not found." << std::endl;
        }

#ifndef NDEBUG
        blockCount = 0;
        groupCount = 0;
#endif
    }

    void RegisterCallbacks(ExtractorCallbacks * em) {
        externalMemory = em;
    }

    void RegisterLUAState(lua_State *ml) {
        myLuaState = ml;
    }

    ~PBFParser() {
        if(input.is_open())
            input.close();

        // Clean up any leftover ThreadData objects in the queue
        _ThreadData* td;
        while (threadDataQueue->try_pop(td)) {
            delete td;
        }
        google::protobuf::ShutdownProtobufLibrary();

#ifndef NDEBUG
        DEBUG("parsed " << blockCount << " blocks from pbf with " << groupCount << " groups");
#endif
    }

    inline bool Init() {
        _ThreadData initData;
        /** read Header */
        if(!readPBFBlobHeader(input, &initData)) {
            return false;
        }

        if(readBlob(input, &initData)) {
            if(!initData.PBFHeaderBlock.ParseFromArray(&(initData.charBuffer[0]), initData.charBuffer.size() ) ) {
                std::cerr << "[error] Header not parseable!" << std::endl;
                return false;
            }

            for(int i = 0, featureSize = initData.PBFHeaderBlock.required_features_size(); i < featureSize; ++i) {
                const std::string& feature = initData.PBFHeaderBlock.required_features( i );
                bool supported = false;
                if ( "OsmSchema-V0.6" == feature )
                    supported = true;
                else if ( "DenseNodes" == feature )
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

    inline void ReadData() {
        bool keepRunning = true;
        do {
            _ThreadData *threadData = new _ThreadData();
            keepRunning = readNextBlock(input, threadData);

            if (keepRunning)
                threadDataQueue->push(threadData);
            else {
                threadDataQueue->push(NULL); // No more data to read, parse stops when NULL encountered
                delete threadData;
            }
        } while(keepRunning);
    }

    inline void ParseData() {
        while (1) {
            _ThreadData *threadData;
            threadDataQueue->wait_and_pop(threadData);
            if (threadData == NULL) {
                INFO("Parse Data Thread Finished");
                threadDataQueue->push(NULL); // Signal end of data for other threads
                break;
            }

            loadBlock(threadData);

            for(int i = 0, groupSize = threadData->PBFprimitiveBlock.primitivegroup_size(); i < groupSize; ++i) {
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

            delete threadData;
            threadData = NULL;
        }
    }

    inline bool Parse() {
        // Start the read and parse threads
        boost::thread readThread(boost::bind(&PBFParser::ReadData, this));

        //Open several parse threads that are synchronized before call to
        boost::thread parseThread(boost::bind(&PBFParser::ParseData, this));

        // Wait for the threads to finish
        readThread.join();
        parseThread.join();

        return true;
    }

private:

    inline void parseDenseNode(_ThreadData * threadData) {
        const OSMPBF::DenseNodes& dense = threadData->PBFprimitiveBlock.primitivegroup( threadData->currentGroupID ).dense();
        int denseTagIndex = 0;
        int m_lastDenseID = 0;
        int m_lastDenseLatitude = 0;
        int m_lastDenseLongitude = 0;

        ImportNode n;
		
		//TODO: abbruchschwellenwert initialisieren
        for(int i = 0, idSize = dense.id_size(); i < idSize; ++i) {
			n.Clear();
            m_lastDenseID += dense.id( i );
            m_lastDenseLatitude += dense.lat( i );
            m_lastDenseLongitude += dense.lon( i );
            n.id = m_lastDenseID;
            n.lat = 100000*( ( double ) m_lastDenseLatitude * threadData->PBFprimitiveBlock.granularity() +threadData-> PBFprimitiveBlock.lat_offset() ) / NANO;
            n.lon = 100000*( ( double ) m_lastDenseLongitude * threadData->PBFprimitiveBlock.granularity() + threadData->PBFprimitiveBlock.lon_offset() ) / NANO;
            while (denseTagIndex < dense.keys_vals_size()) {
                const int tagValue = dense.keys_vals( denseTagIndex );
                if(tagValue == 0) {
                    ++denseTagIndex;
                    break;
                }
                const int keyValue = dense.keys_vals ( denseTagIndex+1 );
                const std::string & key = threadData->PBFprimitiveBlock.stringtable().s(tagValue).data();
                const std::string & value = threadData->PBFprimitiveBlock.stringtable().s(keyValue).data();
                n.keyVals.Add(key, value);
                denseTagIndex += 2;
            }

			//TODO: store in container and process in bulk
            /** Pass the unpacked node to the LUA call back **/
            try {
                luabind::call_function<int>(
                        myLuaState,
                        "node_function",
                        boost::ref(n)
                );
                if(!externalMemory->nodeFunction(n))
                    std::cerr << "[PBFParser] dense node not parsed" << std::endl;
            } catch (const luabind::error &er) {
                cerr << er.what() << endl;
                lua_State* Ler=er.state();
                report_errors(Ler, -1);
            }
            catch (...) {
                ERR("Unknown error occurred during PBF dense node parsing!");
            }
        }
    }

    inline void parseNode(_ThreadData * ) {
        ERR("Parsing of simple nodes not supported. PBF should use dense nodes");
    }

    inline void parseRelation(_ThreadData * threadData) {
		//TODO: leave early, if relatio is not a restriction
		//TODO: reuse rawRestriction container
        const OSMPBF::PrimitiveGroup& group = threadData->PBFprimitiveBlock.primitivegroup( threadData->currentGroupID );
        for(int i = 0; i < group.relations_size(); ++i ) {
            const OSMPBF::Relation& inputRelation = threadData->PBFprimitiveBlock.primitivegroup( threadData->currentGroupID ).relations(i);
            bool isRestriction = false;
            bool isOnlyRestriction = false;
            for(int k = 0, endOfKeys = inputRelation.keys_size(); k < endOfKeys; ++k) {
                const std::string & key = threadData->PBFprimitiveBlock.stringtable().s(inputRelation.keys(k));
                const std::string & val = threadData->PBFprimitiveBlock.stringtable().s(inputRelation.vals(k));
                if ("type" == key) {
					if( "restriction" == val)
                    	isRestriction = true;
					else 
						break;
                }
                if ("restriction" == key) {
                    if(val.find("only_") == 0)
                        isOnlyRestriction = true;
                }

            }
            if(isRestriction) {
                long long lastRef = 0;
                _RawRestrictionContainer currentRestrictionContainer(isOnlyRestriction);
                for(int rolesIndex = 0; rolesIndex < inputRelation.roles_sid_size(); ++rolesIndex) {
                    std::string role(threadData->PBFprimitiveBlock.stringtable().s( inputRelation.roles_sid( rolesIndex ) ).data());
                    lastRef += inputRelation.memids(rolesIndex);

                    if(false == ("from" == role || "to" == role || "via" == role)) {
                        continue;
                    }

                    switch(inputRelation.types(rolesIndex)) {
                    case 0: //node
                        if("from" == role || "to" == role) //Only via should be a node
                            continue;
                        assert("via" == role);
                        if(UINT_MAX != currentRestrictionContainer.viaNode)
                            currentRestrictionContainer.viaNode = UINT_MAX;
                        assert(UINT_MAX == currentRestrictionContainer.viaNode);
                        currentRestrictionContainer.restriction.viaNode = lastRef;
                        break;
                    case 1: //way
                        assert("from" == role || "to" == role || "via" == role);
                        if("from" == role) {
                            currentRestrictionContainer.fromWay = lastRef;
                        }
                        if ("to" == role) {
                            currentRestrictionContainer.toWay = lastRef;
                        }
                        if ("via" == role) {
                            assert(currentRestrictionContainer.restriction.toNode == UINT_MAX);
                            currentRestrictionContainer.viaNode = lastRef;
                        }
                        break;
                    case 2: //relation, not used. relations relating to relations are evil.
                        continue;
                        assert(false);
                        break;

                    default: //should not happen
                        //cout << "unknown";
                        assert(false);
                        break;
                    }
                }
                //                if(UINT_MAX != currentRestriction.viaNode) {
                //                    cout << "restr from " << currentRestriction.from << " via ";
                //                    cout << "node " << currentRestriction.viaNode;
                //                    cout << " to " << currentRestriction.to << endl;
                //                }
                if(!externalMemory->restrictionFunction(currentRestrictionContainer))
                    std::cerr << "[PBFParser] relation not parsed" << std::endl;
            }
        }
    }

    inline void parseWay(_ThreadData * threadData) {
        if( threadData->PBFprimitiveBlock.primitivegroup( threadData->currentGroupID ).ways_size() > 0) {
            _Way w;
            for(int i = 0; i < threadData->PBFprimitiveBlock.primitivegroup( threadData->currentGroupID ).ways_size(); ++i) {
				w.Clear();
                const OSMPBF::Way& inputWay = threadData->PBFprimitiveBlock.primitivegroup( threadData->currentGroupID ).ways( i );
                w.id = inputWay.id();
                unsigned pathNode(0);
                for(int i = 0; i < inputWay.refs_size(); ++i) {
                    pathNode += inputWay.refs(i);
                    w.path.push_back(pathNode);
                }
                assert(inputWay.keys_size() == inputWay.vals_size());
                for(int i = 0; i < inputWay.keys_size(); ++i) {
                    const std::string & key = threadData->PBFprimitiveBlock.stringtable().s(inputWay.keys(i));
                    const std::string & val = threadData->PBFprimitiveBlock.stringtable().s(inputWay.vals(i));
                    w.keyVals.Add(key, val);
                }


				//TODO: store in container and process in bulk
                /** Pass the unpacked way to the LUA call back **/
                try {
                    luabind::call_function<int>(
                        myLuaState,
                        "way_function",
                        boost::ref(w),
                        w.path.size()
                    );
                    if(!externalMemory->wayFunction(w)) {
                        std::cerr << "[PBFParser] way not parsed" << std::endl;
                    }
                } catch (const luabind::error &er) {
                    lua_State* Ler=er.state();
                    report_errors(Ler, -1);
                    ERR(er.what());
                }
                catch (...) {
                    ERR("Unknown error!");
                }
            }
        }
    }

    inline void loadGroup(_ThreadData * threadData) {
#ifndef NDEBUG
        ++groupCount;
#endif
		
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

    inline void loadBlock(_ThreadData * threadData) {
#ifndef NDEBUG
        ++blockCount;
#endif
        threadData->currentGroupID = 0;
        threadData->currentEntityID = 0;
    }

    /* Reverses Network Byte Order into something usable */
    inline unsigned swapEndian(unsigned x) const {
        if(getMachineEndianness() == LittleEndian)
            return ( (x>>24) | ((x<<8) & 0x00FF0000) | ((x>>8) & 0x0000FF00) | (x<<24) );
        return x;
    }

    inline bool readPBFBlobHeader(std::fstream& stream, _ThreadData * threadData) {
        int size(0);
        stream.read((char *)&size, sizeof(int));
        size = swapEndian(size);
        if(stream.eof()) {
            return false;
        }
        if ( size > MAX_BLOB_HEADER_SIZE || size < 0 ) {
            return false;
        }
        char *data = new char[size];
        stream.read(data, size*sizeof(data[0]));

        bool dataSuccessfullyParsed = (threadData->PBFBlobHeader).ParseFromArray( data, size);
        delete[] data;
        return dataSuccessfullyParsed;
    }

    inline bool unpackZLIB(std::fstream &, _ThreadData * threadData) {
        unsigned rawSize = threadData->PBFBlob.raw_size();
        char* unpackedDataArray = new char[rawSize];
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
            delete[] unpackedDataArray;
            return false;
        }

        ret = inflate( &compressedDataStream, Z_FINISH );
        if ( ret != Z_STREAM_END ) {
            std::cerr << "[error] failed to inflate zlib stream" << std::endl;
            std::cerr << "[error] Error type: " << ret << std::endl;
            delete[] unpackedDataArray;
            return false;
        }

        ret = inflateEnd( &compressedDataStream );
        if ( ret != Z_OK ) {
            std::cerr << "[error] failed to deinit zlib stream" << std::endl;
            delete[] unpackedDataArray;
            return false;
        }

        threadData->charBuffer.clear(); threadData->charBuffer.resize(rawSize);
		std::copy(unpackedDataArray, unpackedDataArray + rawSize, threadData->charBuffer.begin());
        delete[] unpackedDataArray;
        return true;
    }

    inline bool unpackLZMA(std::fstream &, _ThreadData * ) const {
        return false;
    }

    inline bool readBlob(std::fstream& stream, _ThreadData * threadData) {
        if(stream.eof())
            return false;

        const int size = threadData->PBFBlobHeader.datasize();
        if ( size < 0 || size > MAX_BLOB_SIZE ) {
            std::cerr << "[error] invalid Blob size:" << size << std::endl;
            return false;
        }

        char* data = new char[size];
        stream.read(data, sizeof(data[0])*size);

        if ( !threadData->PBFBlob.ParseFromArray( data, size ) ) {
            std::cerr << "[error] failed to parse blob" << std::endl;
            delete[] data;
            return false;
        }

        if ( threadData->PBFBlob.has_raw() ) {
            const std::string& data = threadData->PBFBlob.raw();
            threadData->charBuffer.clear();
            threadData->charBuffer.resize( data.size() );
			std::copy(data.begin(), data.end(), threadData->charBuffer.begin());
        } else if ( threadData->PBFBlob.has_zlib_data() ) {
            if ( !unpackZLIB(stream, threadData) ) {
                std::cerr << "[error] zlib data encountered that could not be unpacked" << std::endl;
	            delete[] data;
                return false;
            }
        } else if ( threadData->PBFBlob.has_lzma_data() ) {
            if ( !unpackLZMA(stream, threadData) )
                std::cerr << "[error] lzma data encountered that could not be unpacked" << std::endl;
            delete[] data;
            return false;
        } else {
            std::cerr << "[error] Blob contains no data" << std::endl;
            delete[] data;
            return false;
        }
        delete[] data;
        return true;
    }

    inline bool readNextBlock(std::fstream& stream, _ThreadData * threadData) {
        if(stream.eof()) {
            return false;
        }

        if ( !readPBFBlobHeader(stream, threadData) ){
            return false;
        }

        if ( threadData->PBFBlobHeader.type() != "OSMData" ) {
            return false;
        }

        if ( !readBlob(stream, threadData) ) {
            return false;
        }

        if ( !threadData->PBFprimitiveBlock.ParseFromArray( &(threadData->charBuffer[0]), threadData-> charBuffer.size() ) ) {
            ERR("failed to parse PrimitiveBlock");
            return false;
        }
        return true;
    }

	//TODO: gets called multiple times, save information instead of recomputing.
    inline Endianness getMachineEndianness() const {
        int i(1);
        char *p = (char *) &i;
        if (1 == p[0])
            return LittleEndian;
        return BigEndian;
    }

    static const int NANO = 1000 * 1000 * 1000;
    static const int MAX_BLOB_HEADER_SIZE = 64 * 1024;
    static const int MAX_BLOB_SIZE = 32 * 1024 * 1024;

#ifndef NDEBUG
    /* counting the number of read blocks and groups */
    unsigned groupCount;
    unsigned blockCount;
#endif
	
    ExtractorCallbacks * externalMemory;
    /* the input stream to parse */
    std::fstream input;

    /* ThreadData Queue */
    boost::shared_ptr<ConcurrentQueue < _ThreadData* > > threadDataQueue;

    lua_State *myLuaState;
};

#endif /* PBFPARSER_H_ */

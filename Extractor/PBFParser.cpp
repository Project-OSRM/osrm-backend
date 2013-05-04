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

#include "PBFParser.h"

PBFParser::PBFParser(const char * fileName, ExtractorCallbacks* ec, ScriptingEnvironment& se) : BaseParser( ec, se ) {
	GOOGLE_PROTOBUF_VERIFY_VERSION;
	//TODO: What is the bottleneck here? Filling the queue or reading the stuff from disk?
	//NOTE: With Lua scripting, it is parsing the stuff. I/O is virtually for free.
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

PBFParser::~PBFParser() {
	if(input.is_open()) {
		input.close();
	}

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

inline bool PBFParser::ReadHeader() {
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
			if ( "OsmSchema-V0.6" == feature ) {
				supported = true;
			}
			else if ( "DenseNodes" == feature ) {
				supported = true;
			}
			
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

inline void PBFParser::ReadData() {
	bool keepRunning = true;
	do {
		_ThreadData *threadData = new _ThreadData();
		keepRunning = readNextBlock(input, threadData);

		if (keepRunning) {
			threadDataQueue->push(threadData);
		} else {
			threadDataQueue->push(NULL); // No more data to read, parse stops when NULL encountered
			delete threadData;
		}
	} while(keepRunning);
}

inline void PBFParser::ParseData() {
	while (true) {
		_ThreadData *threadData;
		threadDataQueue->wait_and_pop(threadData);
		if( NULL==threadData ) {
			INFO("Parse Data Thread Finished");
			threadDataQueue->push(NULL); // Signal end of data for other threads
			break;
		}

		loadBlock(threadData);

		for(int i = 0, groupSize = threadData->PBFprimitiveBlock.primitivegroup_size(); i < groupSize; ++i) {
			threadData->currentGroupID = i;
			loadGroup(threadData);

			if(threadData->entityTypeIndicator == TypeNode) {
				parseNode(threadData);
			}
			if(threadData->entityTypeIndicator == TypeWay) {
				parseWay(threadData);
			}
			if(threadData->entityTypeIndicator == TypeRelation) {
				parseRelation(threadData);
			}
			if(threadData->entityTypeIndicator == TypeDenseNode) {
				parseDenseNode(threadData);
			}
		}

		delete threadData;
		threadData = NULL;
	}
}

inline bool PBFParser::Parse() {
	// Start the read and parse threads
	boost::thread readThread(boost::bind(&PBFParser::ReadData, this));

	//Open several parse threads that are synchronized before call to
	boost::thread parseThread(boost::bind(&PBFParser::ParseData, this));

	// Wait for the threads to finish
	readThread.join();
	parseThread.join();

	return true;
}

inline void PBFParser::parseDenseNode(_ThreadData * threadData) {
	const OSMPBF::DenseNodes& dense = threadData->PBFprimitiveBlock.primitivegroup( threadData->currentGroupID ).dense();
	int denseTagIndex = 0;
	int64_t m_lastDenseID = 0;
	int64_t m_lastDenseLatitude = 0;
	int64_t m_lastDenseLongitude = 0;

	ImportNode n;
	std::vector<ImportNode> extracted_nodes_vector;
	const int number_of_nodes = dense.id_size();
	extracted_nodes_vector.reserve(number_of_nodes);
	for(int i = 0; i < number_of_nodes; ++i) {
		n.Clear();
		m_lastDenseID += dense.id( i );
		m_lastDenseLatitude += dense.lat( i );
		m_lastDenseLongitude += dense.lon( i );
		n.id = m_lastDenseID;
		n.lat = 100000*( ( double ) m_lastDenseLatitude * threadData->PBFprimitiveBlock.granularity() + threadData->PBFprimitiveBlock.lat_offset() ) / NANO;
		n.lon = 100000*( ( double ) m_lastDenseLongitude * threadData->PBFprimitiveBlock.granularity() + threadData->PBFprimitiveBlock.lon_offset() ) / NANO;
		while (denseTagIndex < dense.keys_vals_size()) {
			const int tagValue = dense.keys_vals( denseTagIndex );
			if( 0==tagValue ) {
				++denseTagIndex;
				break;
			}
			const int keyValue = dense.keys_vals ( denseTagIndex+1 );
			const std::string & key = threadData->PBFprimitiveBlock.stringtable().s(tagValue).data();
			const std::string & value = threadData->PBFprimitiveBlock.stringtable().s(keyValue).data();
			n.keyVals.Add(key, value);
			denseTagIndex += 2;
		}
		extracted_nodes_vector.push_back(n);
	}

#pragma omp parallel for schedule ( guided )
	for(int i = 0; i < number_of_nodes; ++i) {
	    ImportNode &n = extracted_nodes_vector[i];
	    ParseNodeInLua( n, scriptingEnvironment.getLuaStateForThreadID(omp_get_thread_num()) );
	}

	BOOST_FOREACH(ImportNode &n, extracted_nodes_vector) {
	    extractor_callbacks->nodeFunction(n);
	}
}

inline void PBFParser::parseNode(_ThreadData * ) {
	ERR("Parsing of simple nodes not supported. PBF should use dense nodes");
}

inline void PBFParser::parseRelation(_ThreadData * threadData) {
	//TODO: leave early, if relation is not a restriction
	//TODO: reuse rawRestriction container
	if( !use_turn_restrictions ) {
		return;
	}
	const OSMPBF::PrimitiveGroup& group = threadData->PBFprimitiveBlock.primitivegroup( threadData->currentGroupID );
	for(int i = 0; i < group.relations_size(); ++i ) {
		std::string except_tag_string;
		const OSMPBF::Relation& inputRelation = threadData->PBFprimitiveBlock.primitivegroup( threadData->currentGroupID ).relations(i);
		bool isRestriction = false;
		bool isOnlyRestriction = false;
		for(int k = 0, endOfKeys = inputRelation.keys_size(); k < endOfKeys; ++k) {
			const std::string & key = threadData->PBFprimitiveBlock.stringtable().s(inputRelation.keys(k));
			const std::string & val = threadData->PBFprimitiveBlock.stringtable().s(inputRelation.vals(k));
			if ("type" == key) {
				if( "restriction" == val) {
					isRestriction = true;
				} else {
					break;
				}
			}
			if ("restriction" == key) {
				if(val.find("only_") == 0) {
					isOnlyRestriction = true;
				}
			}
			if ("except" == key) {
				except_tag_string = val;
			}
		}

		if( isRestriction && ShouldIgnoreRestriction(except_tag_string) ) {
			continue;
		}

		if(isRestriction) {
			int64_t lastRef = 0;
			_RawRestrictionContainer currentRestrictionContainer(isOnlyRestriction);
			for(int rolesIndex = 0; rolesIndex < inputRelation.roles_sid_size(); ++rolesIndex) {
				std::string role(threadData->PBFprimitiveBlock.stringtable().s( inputRelation.roles_sid( rolesIndex ) ).data());
				lastRef += inputRelation.memids(rolesIndex);

				if(!("from" == role || "to" == role || "via" == role)) {
					continue;
				}

				switch(inputRelation.types(rolesIndex)) {
				case 0: //node
					if("from" == role || "to" == role) { //Only via should be a node
						continue;
					}
					assert("via" == role);
					if(UINT_MAX != currentRestrictionContainer.viaNode) {
						currentRestrictionContainer.viaNode = UINT_MAX;
					}
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
			if(!extractor_callbacks->restrictionFunction(currentRestrictionContainer)) {
				std::cerr << "[PBFParser] relation not parsed" << std::endl;
			}
		}
	}
}

inline void PBFParser::parseWay(_ThreadData * threadData) {
	std::vector<ExtractionWay> waysToParse;
	const int number_of_ways = threadData->PBFprimitiveBlock.primitivegroup( threadData->currentGroupID ).ways_size();
	waysToParse.reserve(number_of_ways);
	for(int i = 0; i < number_of_ways; ++i) {
		ExtractionWay w;
		const OSMPBF::Way& inputWay = threadData->PBFprimitiveBlock.primitivegroup( threadData->currentGroupID ).ways( i );
		w.id = inputWay.id();
		unsigned pathNode(0);
		const int number_of_referenced_nodes = inputWay.refs_size();
		for(int i = 0; i < number_of_referenced_nodes; ++i) {
			pathNode += inputWay.refs(i);
			w.path.push_back(pathNode);
		}
		assert(inputWay.keys_size() == inputWay.vals_size());
		const int number_of_keys = inputWay.keys_size();
		for(int i = 0; i < number_of_keys; ++i) {
			const std::string & key = threadData->PBFprimitiveBlock.stringtable().s(inputWay.keys(i));
			const std::string & val = threadData->PBFprimitiveBlock.stringtable().s(inputWay.vals(i));
			w.keyVals.Add(key, val);
		}
		waysToParse.push_back(w);
	}

#pragma omp parallel for schedule ( guided )
	for(int i = 0; i < number_of_ways; ++i) {
	    ExtractionWay & w = waysToParse[i];
	    ParseWayInLua( w, scriptingEnvironment.getLuaStateForThreadID(omp_get_thread_num()) );        
	}

	BOOST_FOREACH(ExtractionWay & w, waysToParse) {
	    extractor_callbacks->wayFunction(w);
	}
}

inline void PBFParser::loadGroup(_ThreadData * threadData) {
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

inline void PBFParser::loadBlock(_ThreadData * threadData) {
#ifndef NDEBUG
	++blockCount;
#endif
	threadData->currentGroupID = 0;
	threadData->currentEntityID = 0;
}

inline bool PBFParser::readPBFBlobHeader(std::fstream& stream, _ThreadData * threadData) {
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

inline bool PBFParser::unpackZLIB(std::fstream &, _ThreadData * threadData) {
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

inline bool PBFParser::unpackLZMA(std::fstream &, _ThreadData * ) {
	return false;
}

inline bool PBFParser::readBlob(std::fstream& stream, _ThreadData * threadData) {
	if(stream.eof()) {
		return false;
	}
	
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
		if ( !unpackLZMA(stream, threadData) ) {
			std::cerr << "[error] lzma data encountered that could not be unpacked" << std::endl;
		}
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

bool PBFParser::readNextBlock(std::fstream& stream, _ThreadData * threadData) {
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

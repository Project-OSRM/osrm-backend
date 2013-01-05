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

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/ref.hpp>

#include <osmpbf/fileformat.pb.h>
#include <osmpbf/osmformat.pb.h>

#include <zlib.h>

#include "../typedefs.h"
#include "../DataStructures/HashTable.h"
#include "../DataStructures/ConcurrentQueue.h"
#include "../Util/MachineInfo.h"
#include "../Util/OpenMPWrapper.h"

#include "BaseParser.h"
#include "ExtractorCallbacks.h"
#include "ExtractorStructs.h"
#include "ScriptingEnvironment.h"

class PBFParser : public BaseParser<ExtractorCallbacks, _Node, _RawRestrictionContainer, _Way> {
    
    enum EntityType {
        TypeNode = 1,
        TypeWay = 2,
        TypeRelation = 4,
        TypeDenseNode = 8
    } ;
    
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
    PBFParser(const char * fileName);
    virtual ~PBFParser();
    
    void RegisterCallbacks(ExtractorCallbacks * em);
    void RegisterScriptingEnvironment(ScriptingEnvironment & _se);
    
    inline bool Init();
	inline bool Parse();
    
private:
    inline void ReadData();
    inline void ParseData();
    inline void parseDenseNode(_ThreadData * threadData);
    inline void parseNode(_ThreadData * );
    inline void parseRelation(_ThreadData * threadData);
    inline void parseWay(_ThreadData * threadData);
    
    inline void loadGroup(_ThreadData * threadData);
    inline void loadBlock(_ThreadData * threadData);
    inline bool readPBFBlobHeader(std::fstream& stream, _ThreadData * threadData);
    inline bool unpackZLIB(std::fstream &, _ThreadData * threadData);
    inline bool unpackLZMA(std::fstream &, _ThreadData * );
    inline bool readBlob(std::fstream& stream, _ThreadData * threadData) ;
    inline bool readNextBlock(std::fstream& stream, _ThreadData * threadData);
    
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
    ScriptingEnvironment scriptingEnvironment;

    std::vector<std::string> restriction_exceptions_vector;
};

#endif /* PBFPARSER_H_ */

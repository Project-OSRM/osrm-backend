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

#ifndef PBFPARSER_H_
#define PBFPARSER_H_

#include "BaseParser.h"

#include "../DataStructures/Coordinate.h"
#include "../DataStructures/HashTable.h"
#include "../DataStructures/ConcurrentQueue.h"
#include "../Util/MachineInfo.h"
#include "../Util/OpenMPWrapper.h"
#include "../Util/OSRMException.h"
#include "../Util/SimpleLogger.h"
#include "../typedefs.h"

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/ref.hpp>

#include <osmpbf/fileformat.pb.h>
#include <osmpbf/osmformat.pb.h>

#include <zlib.h>

class PBFParser : public BaseParser {

    enum EntityType {
        TypeDummy     = 0,
        TypeNode      = 1,
        TypeWay       = 2,
        TypeRelation  = 4,
        TypeDenseNode = 8
    };

    struct _ThreadData {
        int currentGroupID;
        int currentEntityID;
        EntityType entityTypeIndicator;

        OSMPBF::BlobHeader PBFBlobHeader;
        OSMPBF::Blob PBFBlob;

        OSMPBF::HeaderBlock PBFHeaderBlock;
        OSMPBF::PrimitiveBlock PBFprimitiveBlock;

        std::vector<char> charBuffer;
    };

public:
    PBFParser(const char * fileName, ExtractorCallbacks* ec, ScriptingEnvironment& se);
    virtual ~PBFParser();

    inline bool ReadHeader();
	inline bool Parse();

private:
    inline void ReadData();
    inline void ParseData();
    inline void parseDenseNode  (_ThreadData * threadData);
    inline void parseNode       (_ThreadData * threadData);
    inline void parseRelation   (_ThreadData * threadData);
    inline void parseWay        (_ThreadData * threadData);

    inline void loadGroup       (_ThreadData * threadData);
    inline void loadBlock       (_ThreadData * threadData);
    inline bool readPBFBlobHeader(std::fstream & stream, _ThreadData * threadData);
    inline bool unpackZLIB       (std::fstream & stream, _ThreadData * threadData);
    inline bool unpackLZMA       (std::fstream & stream, _ThreadData * threadData);
    inline bool readBlob         (std::fstream & stream, _ThreadData * threadData);
    inline bool readNextBlock    (std::fstream & stream, _ThreadData * threadData);

    static const int NANO = 1000 * 1000 * 1000;
    static const int MAX_BLOB_HEADER_SIZE = 64 * 1024;
    static const int MAX_BLOB_SIZE = 32 * 1024 * 1024;

#ifndef NDEBUG
    /* counting the number of read blocks and groups */
    unsigned groupCount;
    unsigned blockCount;
#endif

    std::fstream input;     // the input stream to parse
    boost::shared_ptr<ConcurrentQueue < _ThreadData* > > threadDataQueue;
};

#endif /* PBFPARSER_H_ */

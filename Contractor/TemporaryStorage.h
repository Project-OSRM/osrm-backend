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

#ifndef TEMPORARYSTORAGE_H_
#define TEMPORARYSTORAGE_H_

#include <vector>
#include <fstream>

#include <boost/assert.hpp>
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include "../Util/OSRMException.h"
#include "../Util/SimpleLogger.h"
#include "../typedefs.h"

//This is one big workaround for latest boost renaming woes.

#if BOOST_FILESYSTEM_VERSION < 3
#warning Boost Installation with Filesystem3 missing, activating workaround
#include <cstdio>
namespace boost {
namespace filesystem {
inline path temp_directory_path() {
	char * buffer;
	buffer = tmpnam (NULL);

	return path(buffer);
}

inline path unique_path(const path&) {
	return temp_directory_path();
}

}
}

#endif

#ifndef BOOST_FILESYSTEM_VERSION
#define BOOST_FILESYSTEM_VERSION 3
#endif
/**
 * This class implements a singleton file storage for temporary data.
 * temporary slots can be accessed by other objects through an int
 * On deallocation every slot gets deallocated
 *
 * Access is sequential, which means, that there is no random access
 * -> Data is written in first phase and reread in second.
 */

static boost::filesystem::path tempDirectory;
static std::string TemporaryFilePattern("OSRM-%%%%-%%%%-%%%%");
class TemporaryStorage {
public:
    static TemporaryStorage & GetInstance();
    virtual ~TemporaryStorage();

    int allocateSlot();
    void deallocateSlot(int slotID);
    void writeToSlot(int slotID, char * pointer, std::streamsize size);
    void readFromSlot(int slotID, char * pointer, std::streamsize size);
    //returns the number of free bytes
    unsigned getFreeBytesOnTemporaryDevice();
    boost::filesystem::fstream::pos_type tell(int slotID);
    void seek(int slotID, boost::filesystem::fstream::pos_type);
    void removeAll();
private:
    TemporaryStorage();
    TemporaryStorage(TemporaryStorage const &){};
    TemporaryStorage& operator=(TemporaryStorage const &) {
        return *this;
    }
    void abort(boost::filesystem::filesystem_error& e);

    struct StreamData {
        bool writeMode;
        boost::filesystem::path pathToTemporaryFile;
        boost::shared_ptr<boost::filesystem::fstream> streamToTemporaryFile;
        boost::shared_ptr<boost::mutex> readWriteMutex;
        StreamData() :
            writeMode(true),
            pathToTemporaryFile (boost::filesystem::unique_path(tempDirectory.append(TemporaryFilePattern.begin(), TemporaryFilePattern.end()))),
            streamToTemporaryFile(new boost::filesystem::fstream(pathToTemporaryFile, std::ios::in | std::ios::out | std::ios::trunc | std::ios::binary)),
            readWriteMutex(new boost::mutex)
        {
            if(streamToTemporaryFile->fail()) {
                throw OSRMException("temporary file could not be created");
            }
        }
    };
    //vector of file streams that is used to store temporary data
    std::vector<StreamData> vectorOfStreamDatas;
    boost::mutex mutex;
};

#endif /* TEMPORARYSTORAGE_H_ */

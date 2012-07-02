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

#ifndef TEMPORARYSTORAGE_H_
#define TEMPORARYSTORAGE_H_

#include <vector>
#include <fstream>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include "../typedefs.h"

/**
 * This class implements a singleton file storage for temporary data.
 * temporary slots can be accessed by other objects through an int
 * On deallocation every slot gets deallocated
 *
 * Access is sequential, which means, that there is no random access
 * -> Data is written in first phase and reread in second.
 */
static boost::filesystem3::path tempDirectory;
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
    boost::filesystem3::fstream::pos_type tell(int slotID);
    void seek(int slotID, boost::filesystem3::fstream::pos_type);
    void removeAll();
private:
    TemporaryStorage();
    TemporaryStorage(TemporaryStorage const &){};
    TemporaryStorage& operator=(TemporaryStorage const &) {
        return *this;
    }
    void abort(boost::filesystem3::filesystem_error& e);

    ;

    struct StreamData {
        bool writeMode;
        boost::filesystem3::path pathToTemporaryFile;
        boost::shared_ptr<boost::filesystem3::fstream> streamToTemporaryFile;
        boost::shared_ptr<boost::mutex> readWriteMutex;
        StreamData() :
            writeMode(true),
            pathToTemporaryFile (boost::filesystem3::unique_path(tempDirectory.append(TemporaryFilePattern.begin(), TemporaryFilePattern.end()))),
            streamToTemporaryFile(new boost::filesystem3::fstream(pathToTemporaryFile, std::ios::in | std::ios::out | std::ios::trunc | std::ios::binary)),
            readWriteMutex(new boost::mutex)
        {
            if(streamToTemporaryFile->fail())
                ERR("Aborting, because temporary file at " << pathToTemporaryFile << " could not be created");
        }
    };
    //vector of file streams that is used to store temporary data
    std::vector<StreamData> vectorOfStreamDatas;
    boost::mutex mutex;
};

#endif /* TEMPORARYSTORAGE_H_ */

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

#include "../Util/BoostFileSystemFix.h"
#include "../Util/OSRMException.h"
#include "../Util/SimpleLogger.h"
#include "../typedefs.h"

#include <boost/assert.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/thread/mutex.hpp>

#include <cstdint>

#include <vector>
#include <fstream>
#include <memory>

struct StreamData
{
    bool write_mode;
    boost::filesystem::path temp_path;
    std::shared_ptr<boost::filesystem::fstream> temp_file;
    std::shared_ptr<boost::mutex> readWriteMutex;
    std::vector<char> buffer;

    StreamData();
};

// This class implements a singleton file storage for temporary data.
// temporary slots can be accessed by other objects through an int
// On deallocation every slot gets deallocated
//
// Access is sequential, which means, that there is no random access
// -> Data is written in first phase and reread in second.

static boost::filesystem::path temp_directory;
static std::string TemporaryFilePattern("OSRM-%%%%-%%%%-%%%%");
class TemporaryStorage
{
  public:
    static TemporaryStorage &GetInstance();
    virtual ~TemporaryStorage();

    int AllocateSlot();
    void DeallocateSlot(const int slot_id);
    void WriteToSlot(const int slot_id, char *pointer, const std::size_t size);
    void ReadFromSlot(const int slot_id, char *pointer, const std::size_t size);
    // returns the number of free bytes
    uint64_t GetFreeBytesOnTemporaryDevice();
    boost::filesystem::fstream::pos_type Tell(const int slot_id);
    void RemoveAll();

  private:
    TemporaryStorage();
    TemporaryStorage(TemporaryStorage const &) {};

    TemporaryStorage &operator=(TemporaryStorage const &) { return *this; }

    void Abort(const boost::filesystem::filesystem_error &e);
    void CheckIfTemporaryDeviceFull();

    // vector of file streams that is used to store temporary data
    boost::mutex mutex;
    std::vector<StreamData> stream_data_list;
};

#endif /* TEMPORARYSTORAGE_H_ */

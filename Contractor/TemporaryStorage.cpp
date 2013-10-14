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

#include "TemporaryStorage.h"

TemporaryStorage::TemporaryStorage() {
    tempDirectory = boost::filesystem::temp_directory_path();
}

TemporaryStorage & TemporaryStorage::GetInstance(){
    static TemporaryStorage runningInstance;
    return runningInstance;
}

TemporaryStorage::~TemporaryStorage() {
    removeAll();
}

void TemporaryStorage::removeAll() {
    boost::mutex::scoped_lock lock(mutex);
    for(unsigned slot_id = 0; slot_id < vectorOfStreamDatas.size(); ++slot_id) {
        deallocateSlot(slot_id);
    }
    vectorOfStreamDatas.clear();
}

int TemporaryStorage::allocateSlot() {
    boost::mutex::scoped_lock lock(mutex);
    try {
        vectorOfStreamDatas.push_back(StreamData());
        //SimpleLogger().Write() << "created new temporary file: " << vectorOfStreamDatas.back().pathToTemporaryFile;
    } catch(boost::filesystem::filesystem_error & e) {
        abort(e);
    }
    return vectorOfStreamDatas.size() - 1;
}

void TemporaryStorage::deallocateSlot(int slotID) {
    try {
        StreamData & data = vectorOfStreamDatas[slotID];
        boost::mutex::scoped_lock lock(*data.readWriteMutex);
        if(!boost::filesystem::exists(data.pathToTemporaryFile)) {
            return;
        }
        if(data.streamToTemporaryFile->is_open()) {
            data.streamToTemporaryFile->close();
        }

        boost::filesystem::remove(data.pathToTemporaryFile);
    } catch(boost::filesystem::filesystem_error & e) {
        abort(e);
    }
}

void TemporaryStorage::writeToSlot(int slotID, char * pointer, std::streamsize size) {
    try {
        StreamData & data = vectorOfStreamDatas[slotID];
        boost::mutex::scoped_lock lock(*data.readWriteMutex);
        BOOST_ASSERT_MSG(
            data.writeMode,
            "Writing after first read is not allowed"
        );
        data.streamToTemporaryFile->write(pointer, size);
    } catch(boost::filesystem::filesystem_error & e) {
        abort(e);
    }
}
void TemporaryStorage::readFromSlot(int slotID, char * pointer, std::streamsize size) {
    try {
        StreamData & data = vectorOfStreamDatas[slotID];
        boost::mutex::scoped_lock lock(*data.readWriteMutex);
        if(data.writeMode) {
            data.writeMode = false;
            data.streamToTemporaryFile->seekg(0, data.streamToTemporaryFile->beg);
        }
        data.streamToTemporaryFile->read(pointer, size);
    } catch(boost::filesystem::filesystem_error & e) {
        abort(e);
    }
}

unsigned TemporaryStorage::getFreeBytesOnTemporaryDevice() {
    boost::filesystem::space_info tempSpaceInfo;
    try {
        tempSpaceInfo = boost::filesystem::space(tempDirectory);
    } catch(boost::filesystem::filesystem_error & e) {
        abort(e);
    }
    return tempSpaceInfo.available;
}

boost::filesystem::fstream::pos_type TemporaryStorage::tell(int slotID) {
    boost::filesystem::fstream::pos_type position;
    try {
        StreamData & data = vectorOfStreamDatas[slotID];
        boost::mutex::scoped_lock lock(*data.readWriteMutex);
        position = data.streamToTemporaryFile->tellp();
    } catch(boost::filesystem::filesystem_error & e) {
        abort(e);
   }
    return position;
}

void TemporaryStorage::abort(boost::filesystem::filesystem_error& ) {
    removeAll();
}

void TemporaryStorage::seek(int slotID, boost::filesystem::fstream::pos_type position) {
    try {
        StreamData & data = vectorOfStreamDatas[slotID];
        boost::mutex::scoped_lock lock(*data.readWriteMutex);
        data.streamToTemporaryFile->seekg(position);
    } catch(boost::filesystem::filesystem_error & e) {
        abort(e);
    }
}

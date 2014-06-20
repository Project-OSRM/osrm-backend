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

StreamData::StreamData()
    : write_mode(true),
      temp_path(boost::filesystem::unique_path(temp_directory / TemporaryFilePattern)),
      temp_file(new boost::filesystem::fstream(
          temp_path, std::ios::in | std::ios::out | std::ios::trunc | std::ios::binary)),
      readWriteMutex(std::make_shared<boost::mutex>())
{
    if (temp_file->fail())
    {
        throw OSRMException("temporary file could not be created");
    }
}

TemporaryStorage::TemporaryStorage() { temp_directory = boost::filesystem::temp_directory_path(); }

TemporaryStorage &TemporaryStorage::GetInstance()
{
    static TemporaryStorage static_instance;
    return static_instance;
}

TemporaryStorage::~TemporaryStorage() { RemoveAll(); }

void TemporaryStorage::RemoveAll()
{
    boost::mutex::scoped_lock lock(mutex);
    for (unsigned slot_id = 0; slot_id < stream_data_list.size(); ++slot_id)
    {
        DeallocateSlot(slot_id);
    }
    stream_data_list.clear();
}

int TemporaryStorage::AllocateSlot()
{
    boost::mutex::scoped_lock lock(mutex);
    try { stream_data_list.push_back(StreamData()); }
    catch (boost::filesystem::filesystem_error &e) { Abort(e); }
    CheckIfTemporaryDeviceFull();
    return stream_data_list.size() - 1;
}

void TemporaryStorage::DeallocateSlot(const int slot_id)
{
    try
    {
        StreamData &data = stream_data_list[slot_id];
        boost::mutex::scoped_lock lock(*data.readWriteMutex);
        if (!boost::filesystem::exists(data.temp_path))
        {
            return;
        }
        if (data.temp_file->is_open())
        {
            data.temp_file->close();
        }

        boost::filesystem::remove(data.temp_path);
    }
    catch (boost::filesystem::filesystem_error &e) { Abort(e); }
}

void TemporaryStorage::WriteToSlot(const int slot_id, char *pointer, const std::size_t size)
{
    try
    {
        StreamData &data = stream_data_list[slot_id];
        BOOST_ASSERT(data.write_mode);

        boost::mutex::scoped_lock lock(*data.readWriteMutex);
        BOOST_ASSERT_MSG(data.write_mode, "Writing after first read is not allowed");
        if (1073741824 < data.buffer.size())
        {
            data.temp_file->write(&data.buffer[0], data.buffer.size());
            // data.temp_file->write(pointer, size);
            data.buffer.clear();
            CheckIfTemporaryDeviceFull();
        }
        data.buffer.insert(data.buffer.end(), pointer, pointer + size);
    }
    catch (boost::filesystem::filesystem_error &e) { Abort(e); }
}
void TemporaryStorage::ReadFromSlot(const int slot_id, char *pointer, const std::size_t size)
{
    try
    {
        StreamData &data = stream_data_list[slot_id];
        boost::mutex::scoped_lock lock(*data.readWriteMutex);
        if (data.write_mode)
        {
            data.write_mode = false;
            data.temp_file->write(&data.buffer[0], data.buffer.size());
            data.buffer.clear();
            data.temp_file->seekg(data.temp_file->beg);
            BOOST_ASSERT(data.temp_file->beg == data.temp_file->tellg());
        }
        BOOST_ASSERT(!data.write_mode);
        data.temp_file->read(pointer, size);
    }
    catch (boost::filesystem::filesystem_error &error) { Abort(error); }
}

uint64_t TemporaryStorage::GetFreeBytesOnTemporaryDevice()
{
    uint64_t value = -1;
    try
    {
        boost::filesystem::path path = boost::filesystem::temp_directory_path();
        boost::filesystem::space_info space_info = boost::filesystem::space(path);
        value = space_info.free;
    }
    catch (boost::filesystem::filesystem_error &error) { Abort(error); }
    return value;
}

void TemporaryStorage::CheckIfTemporaryDeviceFull()
{
    boost::filesystem::path path = boost::filesystem::temp_directory_path();
    boost::filesystem::space_info space_info = boost::filesystem::space(path);
    if ((1024 * 1024) > space_info.free)
    {
        throw OSRMException("temporary device is full");
    }
}

boost::filesystem::fstream::pos_type TemporaryStorage::Tell(const int slot_id)
{
    boost::filesystem::fstream::pos_type position;
    try
    {
        StreamData &data = stream_data_list[slot_id];
        boost::mutex::scoped_lock lock(*data.readWriteMutex);
        position = data.temp_file->tellp();
    }
    catch (boost::filesystem::filesystem_error &e) { Abort(e); }
    return position;
}

void TemporaryStorage::Abort(const boost::filesystem::filesystem_error &error)
{
    RemoveAll();
    throw OSRMException(error.what());
}

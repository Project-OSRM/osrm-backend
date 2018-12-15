/*

Copyright (c) 2017, Project OSRM contributors
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

#ifndef STORAGE_HPP
#define STORAGE_HPP

#include "storage/shared_data_index.hpp"
#include "storage/shared_datatype.hpp"
#include "storage/storage_config.hpp"

#include <boost/filesystem/path.hpp>

#include <string>
#include <vector>

namespace osrm
{
namespace storage
{

void populateLayoutFromFile(const boost::filesystem::path &path, storage::BaseDataLayout &layout);

class Storage
{
  public:
    Storage(StorageConfig config);

    int Run(int max_wait, const std::string &name, bool only_metric);
    void PopulateStaticData(const SharedDataIndex &index);
    void PopulateUpdatableData(const SharedDataIndex &index);
    void PopulateLayout(storage::BaseDataLayout &layout,
                        const std::vector<std::pair<bool, boost::filesystem::path>> &files);
    std::string PopulateLayoutWithRTree(storage::BaseDataLayout &layout);
    std::vector<std::pair<bool, boost::filesystem::path>> GetUpdatableFiles();
    std::vector<std::pair<bool, boost::filesystem::path>> GetStaticFiles();

  private:
    StorageConfig config;
};
}
}

#endif

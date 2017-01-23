/*

Copyright (c) 2016, Project OSRM contributors
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

#ifndef PARTITIONER_CONFIG_HPP
#define PARTITIONER_CONFIG_HPP

#include <boost/filesystem/path.hpp>

#include <array>
#include <string>

namespace osrm
{
namespace partition
{

struct PartitionConfig
{
    PartitionConfig() noexcept : requested_num_threads(0) {}

    void UseDefaults()
    {
        std::string basepath = base_path.string();

        const std::string ext = ".osrm";
        const auto pos = basepath.find(ext);
        if (pos != std::string::npos)
        {
            basepath.replace(pos, ext.size(), "");
        }
        else
        {
            // unknown extension
        }

        edge_based_graph_path = basepath + ".osrm.ebg";
        partition_path = basepath + ".osrm.partition";
    }

    // might be changed to the node based graph at some point
    boost::filesystem::path base_path;
    boost::filesystem::path edge_based_graph_path;
    boost::filesystem::path partition_path;

    unsigned requested_num_threads;
};
}
}

#endif // PARTITIONER_CONFIG_HPP

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

#ifndef EXTRACTOR_CONFIG_HPP
#define EXTRACTOR_CONFIG_HPP

#include <boost/filesystem/path.hpp>

#include <array>
#include <string>

#include "storage/io_config.hpp"

namespace osrm
{
namespace extractor
{

struct ExtractorConfig final : storage::IOConfig
{
    ExtractorConfig() noexcept : requested_num_threads(0) {}

    void UseDefaultOutputNames()
    {
        std::string basepath = input_path.string();

        auto pos = std::string::npos;
        std::array<std::string, 5> known_extensions{
            {".osm.bz2", ".osm.pbf", ".osm.xml", ".pbf", ".osm"}};
        for (auto ext : known_extensions)
        {
            pos = basepath.find(ext);
            if (pos != std::string::npos)
            {
                basepath.replace(pos, ext.size(), "");
                break;
            }
        }

        osrm_path = basepath + ".osrm";

        IOConfig::UseDefaultOutputNames();
    }

    boost::filesystem::path input_path;
    boost::filesystem::path profile_path;

    unsigned requested_num_threads;
    unsigned small_component_size;

    bool generate_edge_lookup;

    bool use_metadata;
    bool parse_conditionals;
};
}
}

#endif // EXTRACTOR_CONFIG_HPP

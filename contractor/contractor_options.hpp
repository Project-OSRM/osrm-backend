/*

Copyright (c) 2015, Project OSRM contributors
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

#ifndef CONTRACTOR_OPTIONS_HPP
#define CONTRACTOR_OPTIONS_HPP

#include <boost/filesystem/path.hpp>

#include <string>

enum class return_code : unsigned
{
    ok,
    fail,
    exit
};

struct ContractorConfig
{
    ContractorConfig() noexcept : requested_num_threads(0) {}

    boost::filesystem::path config_file_path;
    boost::filesystem::path osrm_input_path;
    boost::filesystem::path restrictions_path;
    boost::filesystem::path profile_path;

    std::string node_output_path;
    std::string edge_output_path;
    std::string geometry_output_path;
    std::string graph_output_path;
    std::string rtree_nodes_output_path;
    std::string rtree_leafs_output_path;

    unsigned requested_num_threads;
};

struct ContractorOptions
{
    static return_code ParseArguments(int argc, char *argv[], ContractorConfig &extractor_config);

    static void GenerateOutputFilesNames(ContractorConfig &extractor_config);
};

#endif // EXTRACTOR_OPTIONS_HPP

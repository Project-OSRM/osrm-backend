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

#ifndef INI_FILE_UTIL_H
#define INI_FILE_UTIL_H

#include "SimpleLogger.h"

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <regex>
#include <string>

// support old capitalized option names by down-casing them with a regex replace
inline std::string ReadIniFileAndLowerContents(const boost::filesystem::path &path)
{
    boost::filesystem::fstream config_stream(path);
    std::string input_str((std::istreambuf_iterator<char>(config_stream)),
                          std::istreambuf_iterator<char>());
    std::regex regex("\\w+=");

    std::string output = input_str;
    const std::sregex_token_iterator end;
    for (std::sregex_token_iterator i(input_str.begin(), input_str.end(), regex); i != end; ++i)
    {
        std::string match = *i;
        std::string new_regex = *i;
        std::transform(new_regex.begin(), new_regex.end(), new_regex.begin(), ::tolower);
        SimpleLogger().Write() << match << " - " << new_regex;
        output = std::regex_replace(output, std::regex(match), new_regex);
    }
    return output;
}

#endif // INI_FILE_UTIL_H

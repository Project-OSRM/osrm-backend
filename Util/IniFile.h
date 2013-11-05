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

#ifndef INI_FILE_H_
#define INI_FILE_H_

#include "OSRMException.h"
#include "../DataStructures/HashTable.h"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <exception>
#include <iostream>
#include <string>

class IniFile {
public:
    IniFile(const char * config_filename) {
        boost::filesystem::path config_file(config_filename);
        if ( !boost::filesystem::exists( config_file ) ) {
            std::string error = std::string(config_filename) + " not found";
            throw OSRMException(error);
       }
        if ( 0 == boost::filesystem::file_size( config_file ) ) {
            std::string error = std::string(config_filename) + " is empty";
            throw OSRMException(error);
        }

        boost::filesystem::ifstream config( config_file );
        std::string line;
        if (config.is_open()) {
            while ( config.good() )  {
                getline (config,line);
                std::vector<std::string> tokens;
                Tokenize(line, tokens);
                if(2 == tokens.size() ) {
                    parameters.insert(std::make_pair(tokens[0], tokens[1]));
                }
            }
            config.close();
        }
    }

    std::string GetParameter(const std::string & key){
        return parameters.Find(key);
    }

    std::string GetParameter(const std::string & key) const {
        return parameters.Find(key);
    }

    bool Holds(const std::string & key) const {
        return parameters.Holds(key);
    }

    void SetParameter(const char* key, const char* value) {
        SetParameter(std::string(key), std::string(value));
    }

    void SetParameter(const std::string & key, const std::string & value) {
        parameters.insert(std::make_pair(key, value));
    }

private:
    void Tokenize(
        const std::string& str,
        std::vector<std::string>& tokens,
        const std::string& delimiters = "="
    ) {
        std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
        std::string::size_type pos     = str.find_first_of(delimiters, lastPos);

        while (std::string::npos != pos || std::string::npos != lastPos) {
            std::string temp = str.substr(lastPos, pos - lastPos);
            boost::trim(temp);
            tokens.push_back( temp );
            lastPos = str.find_first_not_of(delimiters, pos);
            pos = str.find_first_of(delimiters, lastPos);
        }
    }

    HashTable<std::string, std::string> parameters;
};

#endif /* INI_FILE_H_ */

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

#ifndef OSRM_ENGINE_SIDE_HPP
#define OSRM_ENGINE_SIDE_HPP

#include <string>

namespace osrm
{
namespace engine
{

enum SideValue
{
    DEFAULT,
    OPPOSITE,
    BOTH

};

struct Side
{
    SideValue side;
    static SideValue fromString(const std::string &str)
    {
        if (str == "d")
            return DEFAULT;
        else if (str == "o")
            return OPPOSITE;
        else
            return BOTH;
    }
    static std::string toString(const Side &side)
    {
        switch(side.side)
        {
            case(DEFAULT) :
              return "0";
            case(OPPOSITE) :
              return "d";
            case(BOTH) :
              return "b";
            default :
              //TODO I don't know what to do here.
              return "b";
        }
    }
};

inline bool operator==(const Side lhs, const Side rhs)
{
    return lhs.side == rhs.side;
}
inline bool operator!=(const Side lhs, const Side rhs) { return !(lhs == rhs); }
}
}

#endif

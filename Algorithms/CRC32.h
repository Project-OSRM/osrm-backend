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

#ifndef CRC32_H_
#define CRC32_H_

#include "../Util/SimpleLogger.h"

#include <boost/crc.hpp>  // for boost::crc_32_type
#include <iostream>

class CRC32 {
private:
    unsigned crc;

    typedef boost::crc_optimal<32, 0x1EDC6F41, 0x0, 0x0, true, true> my_crc_32_type;
    typedef unsigned (CRC32::*CRC32CFunctionPtr)(char *str, unsigned len, unsigned crc);

    unsigned SoftwareBasedCRC32(char *str, unsigned len, unsigned crc);
    unsigned SSEBasedCRC32( char *str, unsigned len, unsigned crc);
    unsigned cpuid(unsigned functionInput);
    CRC32CFunctionPtr detectBestCRC32C();
    CRC32CFunctionPtr crcFunction;
public:
    CRC32();
    unsigned operator()(char *str, unsigned len);
    virtual ~CRC32() {};
};

#endif /* CRC32_H_ */

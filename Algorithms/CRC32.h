/*
 open source routing machine
 Copyright (C) Dennis Luxen, others 2010

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU AFFERO General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU Affero General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 or see http://www.gnu.org/licenses/agpl.txt.
 */

#ifndef CRC32_H_
#define CRC32_H_

#include <boost/crc.hpp>  // for boost::crc_32_type
#include <iostream>

class CRC32 {
private:
    unsigned crc;
    unsigned slowcrc_table[1<<8];

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

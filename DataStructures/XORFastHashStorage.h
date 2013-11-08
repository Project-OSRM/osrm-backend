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

#ifndef XORFASTHASHSTORAGE_H_
#define XORFASTHASHSTORAGE_H_

#include "XORFastHash.h"

#include <climits>
#include <vector>
#include <bitset>

template< typename NodeID, typename Key >
class XORFastHashStorage {
public:
    struct HashCell{
        Key key;
        NodeID id;
        unsigned time;
        HashCell() : key(UINT_MAX), id(UINT_MAX), time(UINT_MAX) {}

        HashCell(const HashCell & other) : key(other.key), id(other.id), time(other.time) { }

        inline operator Key() const {
            return key;
        }

        inline void operator=(const Key & keyToInsert) {
            key = keyToInsert;
        }
    };

    XORFastHashStorage( size_t ) : positions(2<<16), currentTimestamp(0) { }

    inline HashCell& operator[]( const NodeID node ) {
        unsigned short position = fastHash(node);
        while((positions[position].time == currentTimestamp) && (positions[position].id != node)){
            ++position %= (2<<16);
        }

        positions[position].id = node;
        positions[position].time = currentTimestamp;
        return positions[position];
    }

    inline void Clear() {
        ++currentTimestamp;
        if(UINT_MAX == currentTimestamp) {
            positions.clear();
            positions.resize((2<<16));
        }
    }

private:
    XORFastHashStorage() : positions(2<<16), currentTimestamp(0) {}
    std::vector<HashCell> positions;
    XORFastHash fastHash;
    unsigned currentTimestamp;
};


#endif /* XORFASTHASHSTORAGE_H_ */


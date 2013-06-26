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


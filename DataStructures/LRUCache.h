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

#ifndef LRUCACHE_H
#define LRUCACHE_H

#include <list>
#include <boost/unordered_map.hpp>

template<typename KeyT, typename ValueT>
class LRUCache {
private:
    struct CacheEntry {
        CacheEntry(KeyT k, ValueT v) : key(k), value(v) {}
        KeyT key;
        ValueT value;
    };
    unsigned capacity;
    std::list<CacheEntry> itemsInCache;
    boost::unordered_map<KeyT, typename std::list<CacheEntry>::iterator > positionMap;
public:
    LRUCache(unsigned c) : capacity(c) {}

    bool Holds(KeyT key) {
        if(positionMap.find(key) != positionMap.end()) {
            return true;
        }
        return false;
    }

    void Insert(const KeyT key, ValueT &value) {
        itemsInCache.push_front(CacheEntry(key, value));
        positionMap.insert(std::make_pair(key, itemsInCache.begin()));
        if(itemsInCache.size() > capacity) {
            positionMap.erase(itemsInCache.back().key);
            itemsInCache.pop_back();
        }
    }

    void Insert(const KeyT key, ValueT value) {
        itemsInCache.push_front(CacheEntry(key, value));
        positionMap.insert(std::make_pair(key, itemsInCache.begin()));
        if(itemsInCache.size() > capacity) {
            positionMap.erase(itemsInCache.back().key);
            itemsInCache.pop_back();
        }
    }

    bool Fetch(const KeyT key, ValueT& result) {
        if(Holds(key)) {
            CacheEntry e = *(positionMap.find(key)->second);
            result = e.value;

            //move to front
            itemsInCache.splice(positionMap.find(key)->second, itemsInCache, itemsInCache.begin());
            positionMap.find(key)->second = itemsInCache.begin();
            return true;
        }
        return false;
    }
    unsigned Size() const {
        return itemsInCache.size();
    }
};
#endif //LRUCACHE_H

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

  Created on: 18.11.2010
  Author: dennis
 */

#ifndef HASHTABLE_H_
#define HASHTABLE_H_

#include <boost/ref.hpp>
#include <boost/unordered_map.hpp>

template<typename keyT, typename valueT>
class HashTable : public boost::unordered_map<keyT, valueT> {
private:
    typedef boost::unordered_map<keyT, valueT> super;
public:
    HashTable() : super() { }

    HashTable(const unsigned size) : super(size) { }

    HashTable &operator=(const HashTable &other) {
        super::operator = (other);
        return *this;
    }

    inline void Add(const keyT& key, const valueT& value){
        super::insert(std::make_pair(key, value));
    }

    inline valueT Find(const keyT& key) const {
        if(super::find(key) == super::end()) {
            return valueT();
        }
        return boost::ref(super::find(key)->second);
    }

    inline bool Holds(const keyT& key) const {
        if(super::find(key) == super::end()) {
            return false;
        }
        return true;
    }
};

#endif /* HASHTABLE_H_ */

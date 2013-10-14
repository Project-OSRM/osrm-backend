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

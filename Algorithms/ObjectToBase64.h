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

#ifndef OBJECTTOBASE64_H_
#define OBJECTTOBASE64_H_

#include "../Util/StringUtil.h"

#include <boost/assert.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/foreach.hpp>

#include <algorithm>
#include <string>
#include <vector>

typedef
        boost::archive::iterators::base64_from_binary<
            boost::archive::iterators::transform_width<const char *, 6, 8>
        > base64_t;

typedef
        boost::archive::iterators::transform_width<
            boost::archive::iterators::binary_from_base64<
            std::string::const_iterator>, 8, 6
        > binary_t;

template<class ObjectT>
static void EncodeObjectToBase64(const ObjectT & object, std::string& encoded) {
    const char * char_ptr_to_object = (const char *)&object;
    std::vector<unsigned char> data(sizeof(object));
    std::copy(
        char_ptr_to_object,
        char_ptr_to_object + sizeof(ObjectT),
        data.begin()
    );

    unsigned char number_of_padded_chars = 0; // is in {0,1,2};
    while(data.size() % 3 != 0)  {
      ++number_of_padded_chars;
      data.push_back(0x00);
    }

    BOOST_ASSERT_MSG(
        0 == data.size() % 3,
        "base64 input data size is not a multiple of 3!"
    );
    encoded.resize(sizeof(ObjectT));
    encoded.assign(
        base64_t( &data[0] ),
        base64_t( &data[0] + (data.size() - number_of_padded_chars) )
    );
    replaceAll(encoded, "+", "-");
    replaceAll(encoded, "/", "_");
}

template<class ObjectT>
static void DecodeObjectFromBase64(const std::string& input, ObjectT & object) {
    try {
    	std::string encoded(input);
        //replace "-" with "+" and "_" with "/"
        replaceAll(encoded, "-", "+");
        replaceAll(encoded, "_", "/");

        std::copy (
            binary_t( encoded.begin() ),
            binary_t( encoded.begin() + encoded.length() - 1),
            (char *)&object
        );

    } catch(...) { }
}

#endif /* OBJECTTOBASE64_H_ */

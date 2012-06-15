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

#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/foreach.hpp>

#include <algorithm>
#include <string>

#include "../Util/StringUtil.h"

typedef
        boost::archive::iterators::base64_from_binary<
        boost::archive::iterators::transform_width<string::const_iterator, 6, 8>
> base64_t;

typedef
        boost::archive::iterators::transform_width<
        boost::archive::iterators::binary_from_base64<string::const_iterator>, 8, 6
        > binary_t;

template<class ToEncodeT>
static void EncodeObjectToBase64(const ToEncodeT & object, std::string& encoded) {
    encoded.clear();
    char * pointerToOriginalObject = (char *)&object;
    encoded = std::string(base64_t(pointerToOriginalObject), base64_t(pointerToOriginalObject+sizeof(ToEncodeT)));
    //replace "+" with "-" and "/" with "_"
    replaceAll(encoded, "+", "-");
    replaceAll(encoded, "/", "_");
}

template<class ToEncodeT>
static void DecodeObjectFromBase64(ToEncodeT & object, const std::string& _encoded) {
    try {
        string encoded(_encoded);
        //replace "-" with "+" and "_" with "/"
        replaceAll(encoded, "-", "+");
        replaceAll(encoded, "_", "/");
        char * pointerToDecodedObject = (char *)&object;
        std::string dec(binary_t(encoded.begin()), binary_t(encoded.begin() + encoded.length() - 1));
        std::copy ( dec.begin(), dec.end(), pointerToDecodedObject );
    } catch(...) {}
}

#endif /* OBJECTTOBASE64_H_ */

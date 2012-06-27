/*
    open source routing machine
    Copyright (C) Dennis Luxen, 2010

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

#ifndef BASE_DESCRIPTOR_H_
#define BASE_DESCRIPTOR_H_

#include <cassert>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "../typedefs.h"
#include "../DataStructures/ExtractorStructs.h"
#include "../DataStructures/HashTable.h"
#include "../Util/StringUtil.h"

#include "../Plugins/RawRouteData.h"

struct _DescriptorConfig {
    _DescriptorConfig() : instructions(true), geometry(true), encodeGeometry(true), z(18) {}
    bool instructions;
    bool geometry;
    bool encodeGeometry;
    unsigned short z;
};

template<class SearchEngineT>
class BaseDescriptor {
public:
    BaseDescriptor() { }
    //Maybe someone can explain the pure virtual destructor thing to me (dennis)
    virtual ~BaseDescriptor() { }
    virtual void Run(http::Reply & reply, const RawRouteData &rawRoute, PhantomNodes &phantomNodes, SearchEngineT &sEngine) = 0;
    virtual void SetConfig(const _DescriptorConfig & config) = 0;
};

#endif /* BASE_DESCRIPTOR_H_ */

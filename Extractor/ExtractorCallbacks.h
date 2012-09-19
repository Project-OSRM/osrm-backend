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

#ifndef EXTRACTORCALLBACKS_H_
#define EXTRACTORCALLBACKS_H_

#include <string>
#include <vector>

#include <cfloat>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/regex.hpp>

#include "ExtractionContainers.h"
#include "ExtractorStructs.h"

class ExtractorCallbacks{
private:
    StringMap * stringMap;
    ExtractionContainers * externalMemory;

    ExtractorCallbacks();
public:
    explicit ExtractorCallbacks(ExtractionContainers * ext, StringMap * strMap);

    ~ExtractorCallbacks();

    /** warning: caller needs to take care of synchronization! */
    bool nodeFunction(_Node &n);

    bool restrictionFunction(_RawRestrictionContainer &r);

    /** warning: caller needs to take care of synchronization! */
    bool wayFunction(_Way &w);

};

#endif /* EXTRACTORCALLBACKS_H_ */

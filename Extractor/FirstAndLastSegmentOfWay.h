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

#ifndef EXTRACTORSTRUCTS_H_
#define EXTRACTORSTRUCTS_H_

#include "../DataStructures/ExternalMemoryNode.h"
#include "../typedefs.h"

#include <limits>
#include <string>

struct FirstAndLastSegmentOfWay
{
    EdgeID wayID;
    NodeID firstStart;
    NodeID firstTarget;
    NodeID lastStart;
    NodeID lastTarget;
    FirstAndLastSegmentOfWay()
        : wayID(std::numeric_limits<unsigned>::max()), firstStart(std::numeric_limits<unsigned>::max()), firstTarget(std::numeric_limits<unsigned>::max()), lastStart(std::numeric_limits<unsigned>::max()),
          lastTarget(std::numeric_limits<unsigned>::max())
    {
    }

    FirstAndLastSegmentOfWay(unsigned w, NodeID fs, NodeID ft, NodeID ls, NodeID lt)
        : wayID(w), firstStart(fs), firstTarget(ft), lastStart(ls), lastTarget(lt)
    {
    }

    static FirstAndLastSegmentOfWay min_value()
    {
        return FirstAndLastSegmentOfWay((std::numeric_limits<unsigned>::min)(),
                                    (std::numeric_limits<unsigned>::min)(),
                                    (std::numeric_limits<unsigned>::min)(),
                                    (std::numeric_limits<unsigned>::min)(),
                                    (std::numeric_limits<unsigned>::min)());
    }
    static FirstAndLastSegmentOfWay max_value()
    {
        return FirstAndLastSegmentOfWay((std::numeric_limits<unsigned>::max)(),
                                    (std::numeric_limits<unsigned>::max)(),
                                    (std::numeric_limits<unsigned>::max)(),
                                    (std::numeric_limits<unsigned>::max)(),
                                    (std::numeric_limits<unsigned>::max)());
    }
};

struct FirstAndLastSegmentOfWayStxxlCompare
{
    using value_type = FirstAndLastSegmentOfWay;
    bool operator()(const FirstAndLastSegmentOfWay &a, const FirstAndLastSegmentOfWay &b) const
    {
        return a.wayID < b.wayID;
    }
    value_type max_value() { return FirstAndLastSegmentOfWay::max_value(); }
    value_type min_value() { return FirstAndLastSegmentOfWay::min_value(); }
};

#endif /* EXTRACTORSTRUCTS_H_ */

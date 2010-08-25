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

#ifndef TYPEDEFS_H_
#define TYPEDEFS_H_

#ifdef __APPLE__
#include <signal.h>
#endif

#include <climits>

using namespace std;

#define VERBOSE(x) x
#define VERBOSE2(x)
#ifdef STXXL_VERBOSE_LEVEL
#undef STXXL_VERBOSE_LEVEL
#endif
#define STXXL_VERBOSE_LEVEL -100

typedef unsigned int NodeID;
typedef unsigned int EdgeID;
typedef unsigned int EdgeWeight;
static const NodeID SPECIAL_NODEID = UINT_MAX;
static const EdgeID SPECIAL_EDGEID = UINT_MAX;

#include "DataStructures/NodeCoords.h"
typedef NodeCoords<NodeID> NodeInfo;
#include "DataStructures/Util.h"
#include "DataStructures/NodeInformationHelpDesk.h"
#include "Contractor/BinaryHeap.h"
#include "Contractor/Contractor.h"
#include "Contractor/ContractionCleanup.h"
typedef ContractionCleanup::Edge::EdgeData EdgeData;
#include "Contractor/DynamicGraph.h"
#include "Contractor/SearchEngine.h"

#endif /* TYPEDEFS_H_ */

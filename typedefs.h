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
#include <iostream>

using namespace std;

#ifdef STXXL_VERBOSE_LEVEL
#undef STXXL_VERBOSE_LEVEL
#endif
#define STXXL_VERBOSE_LEVEL -100

#define INFO(x) do {std::cout << "[info " << __FILE__ << ":" << __LINE__ << "] " << x << std::endl;} while(0);
#define ERR(x) do {std::cerr << "[error " << __FILE__ << ":" << __LINE__ << "] " << x << std::endl; exit(-1);} while(0);
#define WARN(x) do {std::cerr << "[warn " << __FILE__ << ":" << __LINE__ << "] " << x << std::endl;} while(0);

#ifdef NDEBUG
#define DEBUG(x)
#define GUARANTEE(x,y)
#else
#define DEBUG(x) do {std::cout << "[debug " << __FILE__ << ":" << __LINE__ << "] " << x << std::endl;} while(0);
#define GUARANTEE(x,y) do { {do{ if(false == (x)) { ERR(y) } } while(0);} } while(0);
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef HAVE_ROUNDF
#include <boost/math/tr1.hpp>
using namespace boost::math;
#endif

typedef unsigned int NodeID;
typedef unsigned int EdgeID;
typedef unsigned int EdgeWeight;

static const NodeID SPECIAL_NODEID = UINT_MAX;
static const EdgeID SPECIAL_EDGEID = UINT_MAX;

#include "DataStructures/NodeCoords.h"
typedef NodeCoords<NodeID> NodeInfo;
//#include "DataStructures/NodeInformationHelpDesk.h"
//#include "Contractor/ContractionCleanup.h"
//typedef ContractionCleanup::Edge::EdgeData EdgeData;

#endif /* TYPEDEFS_H_ */

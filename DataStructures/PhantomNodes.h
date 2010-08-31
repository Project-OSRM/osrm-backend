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

#ifndef PHANTOMNODES_H_
#define PHANTOMNODES_H_

#include "ExtractorStructs.h"

struct PhantomNodes {
    PhantomNodes() : startNode1(UINT_MAX), startNode2(UINT_MAX), targetNode1(UINT_MAX), targetNode2(UINT_MAX), startRatio(1.), targetRatio(1.) {}
    NodeID startNode1;
    NodeID startNode2;
    NodeID targetNode1;
    NodeID targetNode2;
    double startRatio;
    double targetRatio;
    _Coordinate startCoord;
    _Coordinate targetCoord;
};

#endif /* PHANTOMNODES_H_ */

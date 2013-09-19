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

#ifndef SHARED_DATA_FACADE
#define SHARED_DATA_FACADE

//implements all data storage when shared memory is _NOT_ used

#include "BaseDataFacade.h"

#include "../../DataStructures/StaticGraph.h"

template<class EdgeDataT>
class SharedDataFacade : public BaseDataFacade<EdgeDataT> {

private:

public:

    //search graph access
    unsigned GetNumberOfNodes() const { return 0; }

    unsigned GetNumberOfEdges() const { return 0; }

    unsigned GetOutDegree( const NodeID n ) const { return 0; }

    NodeID GetTarget( const EdgeID e ) const { return 0; }

    EdgeDataT &GetEdgeData( const EdgeID e ) { return EdgeDataT(); }

    const EdgeDataT &GetEdgeData( const EdgeID e ) const { return EdgeDataT(); }

    EdgeID BeginEdges( const NodeID n ) const { return 0; }

    EdgeID EndEdges( const NodeID n ) const { return 0; }

    //searches for a specific edge
    EdgeID FindEdge( const NodeID from, const NodeID to ) const { return 0; }

    EdgeID FindEdgeInEitherDirection(
        const NodeID from,
        const NodeID to
    ) const { return 0; }

    EdgeID FindEdgeIndicateIfReverse(
        const NodeID from,
        const NodeID to,
        bool & result
    ) const { return 0; }

    //node and edge information access
    FixedPointCoordinate GetCoordinateOfNode(
        const unsigned id
    ) const { return FixedPointCoordinate(); };

    TurnInstruction GetTurnInstructionForEdgeID(
        const unsigned id
    ) const { return 0; }

    bool LocateClosestEndPointForCoordinate(
        const FixedPointCoordinate& input_coordinate,
        FixedPointCoordinate& result,
        const unsigned zoom_level = 18
    ) const { return false; }

    bool FindPhantomNodeForCoordinate(
        const FixedPointCoordinate & input_coordinate,
        PhantomNode & resulting_phantom_node,
        const unsigned zoom_level
    ) const { return false; }

    unsigned GetCheckSum() const { return 0; }

    unsigned GetNameIndexFromEdgeID(const unsigned id) const { return 0; };

    void GetName(
        const unsigned name_id,
        std::string & result
    ) const { return; };
};

#endif  // SHARED_DATA_FACADE

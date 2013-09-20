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

#ifndef QUERY_DATA_FACADE_H
#define QUERY_DATA_FACADE_H

//Exposes all data access interfaces to the algorithms via base class ptr

#include "../../Contractor/EdgeBasedGraphFactory.h"
#include "../../DataStructures/Coordinate.h"
#include "../../DataStructures/PhantomNodes.h"
#include "../../DataStructures/TurnInstructions.h"
#include "../../Util/OSRMException.h"
#include "../../Util/StringUtil.h"
#include "../../typedefs.h"

#include <string>

template<class EdgeDataT>
class BaseDataFacade {
public:
    typedef EdgeBasedGraphFactory::EdgeBasedNode RTreeLeaf;
    typedef EdgeDataT EdgeData;
    BaseDataFacade( ) { }
    virtual ~BaseDataFacade() { }

    //search graph access
    virtual unsigned GetNumberOfNodes() const = 0;

    virtual unsigned GetNumberOfEdges() const = 0;

    virtual unsigned GetOutDegree( const NodeID n ) const = 0;

    virtual NodeID GetTarget( const EdgeID e ) const = 0;

    virtual EdgeDataT &GetEdgeData( const EdgeID e ) = 0;

    virtual const EdgeDataT &GetEdgeData( const EdgeID e ) const = 0;

    virtual EdgeID BeginEdges( const NodeID n ) const = 0;

    virtual EdgeID EndEdges( const NodeID n ) const = 0;

    //searches for a specific edge
    virtual EdgeID FindEdge( const NodeID from, const NodeID to ) const = 0;

    virtual EdgeID FindEdgeInEitherDirection(
        const NodeID from,
        const NodeID to
    ) const = 0;

    virtual EdgeID FindEdgeIndicateIfReverse(
        const NodeID from,
        const NodeID to,
        bool & result
    ) const = 0;

    //node and edge information access
    virtual FixedPointCoordinate GetCoordinateOfNode(
        const unsigned id
    ) const = 0;

    virtual unsigned GetOutDegree( const NodeID n ) const = 0;

    virtual NodeID GetTarget( const EdgeID e ) const = 0;

    virtual EdgeDataT &GetEdgeData( const EdgeID e ) = 0;

    virtual bool LocateClosestEndPointForCoordinate(
        const FixedPointCoordinate& input_coordinate,
        FixedPointCoordinate& result,
        const unsigned zoom_level = 18
    ) const  = 0;

    virtual EdgeID EndEdges( const NodeID n ) const = 0;

    //searches for a specific edge
    virtual EdgeID FindEdge( const NodeID from, const NodeID to ) const = 0;


    virtual unsigned GetNameIndexFromEdgeID(const unsigned id) const  = 0;

    std::string GetEscapedNameForNameID(const unsigned name_id) const {
        std::string temporary_string;
        GetName(name_id, temporary_string);
        return HTMLEntitize(temporary_string);
    }

    virtual std::string GetTimestamp() const = 0;
};

#endif // QUERY_DATA_FACADE_H

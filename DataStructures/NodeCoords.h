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

#ifndef _NODE_COORDS_H
#define _NODE_COORDS_H

#include <limits>

template<typename NodeT>
struct NodeCoords {
    typedef unsigned key_type;


    NodeCoords(int _lat, int _lon, NodeT _id) : lat(_lat), lon(_lon), id(_id) {}
    NodeCoords() : lat(UINT_MAX), lon(UINT_MAX), id(UINT_MAX) {}
    int lat;
    int lon;
    unsigned int id;

    static NodeCoords<NodeT> min_value()
    {
        return NodeCoords<NodeT>(0,0,numeric_limits<NodeT>::min());
    }
    static NodeCoords<NodeT> max_value()
    {
        return NodeCoords<NodeT>(numeric_limits<int>::max(), numeric_limits<int>::max(), numeric_limits<NodeT>::max());
    }

};

template<typename NodeT>
bool operator < (const NodeCoords<NodeT> & a, const NodeCoords<NodeT> & b)
{
    return a.id < b.id;
}

struct duplet
{
    typedef int value_type;

    inline value_type operator[](int const N) const { return d[N]; }

    inline bool operator==(duplet const& other) const
                       {
        return this->d[0] == other.d[0] && this->d[1] == other.d[1];
                       }

    inline bool operator!=(duplet const& other) const
                       {
        return this->d[0] != other.d[0] || this->d[1] != other.d[1];
                       }

    friend std::ostream & operator<<(std::ostream & o, duplet const& d)
    {
        return o << "(" << d[0] << "," << d[1] << ")";
    }
    duplet(unsigned _id, int x, int y)
    {
        id = _id;
        d[0] = x;
        d[1] = y;
    }
    unsigned int id;
    value_type d[2];
};

inline double return_dup( duplet d, int k ) { return d[k]; }

#endif //_NODE_COORDS_H

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

#ifndef BINARYHEAP_H_INCLUDED
#define BINARYHEAP_H_INCLUDED

//Not compatible with non contiguous node ids

#include <boost/unordered_map.hpp>

#include <boost/assert.hpp>

#include <algorithm>
#include <limits>
#include <map>
#include <vector>

template< typename NodeID, typename Key >
class ArrayStorage {
public:

    ArrayStorage( size_t size ) : positions( new Key[size] ) {
        memset(positions, 0, size*sizeof(Key));
    }

    ~ArrayStorage() {
        delete[] positions;
    }

    Key &operator[]( NodeID node ) {
        return positions[node];
    }

    void Clear() {}

private:
    Key* positions;
};

template< typename NodeID, typename Key >
class MapStorage {
public:

    MapStorage( size_t ) {}

    Key &operator[]( NodeID node ) {
        return nodes[node];
    }

    void Clear() {
        nodes.clear();
    }

private:
    std::map< NodeID, Key > nodes;

};

template< typename NodeID, typename Key >
class UnorderedMapStorage {
public:

	UnorderedMapStorage( size_t ) {
		//hash table gets 1000 Buckets
		nodes.rehash(1000);
	}

    Key &operator[]( const NodeID node ) {
    	return nodes[node];
    }

    void Clear() {
        nodes.clear();
    }

private:
    boost::unordered_map< NodeID, Key > nodes;
};

template<typename NodeID = unsigned>
struct _SimpleHeapData {
    NodeID parent;
    _SimpleHeapData( NodeID p ) : parent(p) { }
};

template < typename NodeID, typename Key, typename Weight, typename Data, typename IndexStorage = ArrayStorage<NodeID, NodeID> >
class BinaryHeap {
private:
    BinaryHeap( const BinaryHeap& right );
    void operator=( const BinaryHeap& right );
public:
    typedef Weight WeightType;
    typedef Data DataType;

    BinaryHeap( size_t maxID )
    : nodeIndex( maxID ) {
        Clear();
    }

    void Clear() {
        heap.resize( 1 );
        insertedNodes.clear();
        heap[0].weight = std::numeric_limits< Weight >::min();
        nodeIndex.Clear();
    }

    Key Size() const {
        return static_cast<Key>( heap.size() - 1 );
    }

    bool Empty() const {
        return 0 == Size();
    }

    void Insert( NodeID node, Weight weight, const Data &data ) {
        HeapElement element;
        element.index = static_cast<NodeID>(insertedNodes.size());
        element.weight = weight;
        const Key key = static_cast<Key>(heap.size());
        heap.push_back( element );
        insertedNodes.push_back( HeapNode( node, key, weight, data ) );
        nodeIndex[node] = element.index;
        Upheap( key );
        CheckHeap();
    }

    Data& GetData( NodeID node ) {
        const Key index = nodeIndex[node];
        return insertedNodes[index].data;
    }

    Weight& GetKey( NodeID node ) {
        const Key index = nodeIndex[node];
        return insertedNodes[index].weight;
    }

    bool WasRemoved( const NodeID node ) {
        BOOST_ASSERT( WasInserted( node ) );
        const Key index = nodeIndex[node];
        return insertedNodes[index].key == 0;
    }

    bool WasInserted( const NodeID node ) {
        const Key index = nodeIndex[node];
        if ( index >= static_cast<Key> (insertedNodes.size()) )
            return false;
        return insertedNodes[index].node == node;
    }

    NodeID Min() const {
        BOOST_ASSERT( heap.size() > 1 );
        return insertedNodes[heap[1].index].node;
    }

    NodeID DeleteMin() {
        BOOST_ASSERT( heap.size() > 1 );
        const Key removedIndex = heap[1].index;
        heap[1] = heap[heap.size()-1];
        heap.pop_back();
        if ( heap.size() > 1 )
            Downheap( 1 );
        insertedNodes[removedIndex].key = 0;
        CheckHeap();
        return insertedNodes[removedIndex].node;
    }

    void DeleteAll() {
        for ( typename std::vector< HeapElement >::iterator i = heap.begin() + 1, iend = heap.end(); i != iend; ++i )
            insertedNodes[i->index].key = 0;
        heap.resize( 1 );
        heap[0].weight = (std::numeric_limits< Weight >::min)();
    }

    void DecreaseKey( NodeID node, Weight weight ) {
        BOOST_ASSERT( UINT_MAX != node );
        const Key & index = nodeIndex[node];
        Key & key = insertedNodes[index].key;
        BOOST_ASSERT ( key >= 0 );

        insertedNodes[index].weight = weight;
        heap[key].weight = weight;
        Upheap( key );
        CheckHeap();
    }

private:
    class HeapNode {
    public:
        HeapNode() {
        }
        HeapNode( NodeID n, Key k, Weight w, Data d )
        : node( n ), key( k ), weight( w ), data( d ) {
        }

        NodeID node;
        Key key;
        Weight weight;
        Data data;
    };
    struct HeapElement {
        Key index;
        Weight weight;
    };

    std::vector< HeapNode > insertedNodes;
    std::vector< HeapElement > heap;
    IndexStorage nodeIndex;

    void Downheap( Key key ) {
        const Key droppingIndex = heap[key].index;
        const Weight weight = heap[key].weight;
        Key nextKey = key << 1;
        while ( nextKey < static_cast<Key>( heap.size() ) ) {
            const Key nextKeyOther = nextKey + 1;
            if ( ( nextKeyOther < static_cast<Key> ( heap.size() ) )&& ( heap[nextKey].weight > heap[nextKeyOther].weight) )
                nextKey = nextKeyOther;

            if ( weight <= heap[nextKey].weight )
                break;

            heap[key] = heap[nextKey];
            insertedNodes[heap[key].index].key = key;
            key = nextKey;
            nextKey <<= 1;
        }
        heap[key].index = droppingIndex;
        heap[key].weight = weight;
        insertedNodes[droppingIndex].key = key;
    }

    void Upheap( Key key ) {
        const Key risingIndex = heap[key].index;
        const Weight weight = heap[key].weight;
        Key nextKey = key >> 1;
        while ( heap[nextKey].weight > weight ) {
            BOOST_ASSERT( nextKey != 0 );
            heap[key] = heap[nextKey];
            insertedNodes[heap[key].index].key = key;
            key = nextKey;
            nextKey >>= 1;
        }
        heap[key].index = risingIndex;
        heap[key].weight = weight;
        insertedNodes[risingIndex].key = key;
    }

    void CheckHeap() {
#ifndef NDEBUG
        for ( Key i = 2; i < (Key) heap.size(); ++i ) {
            BOOST_ASSERT( heap[i].weight >= heap[i >> 1].weight );
        }
#endif
    }
};

#endif //#ifndef BINARYHEAP_H_INCLUDED

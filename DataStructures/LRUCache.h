#include <climits>
#include <google/sparse_hash_map>
#include <iostream>
#include <list>
#include <vector>

template < class T >
struct Countfn {
    unsigned long operator()( const T &x ) { return 1; }
};

template< class Key, class Data, class Sizefn = Countfn< Data > > class LRUCache {
public:
    typedef std::list< std::pair< Key, Data > > List;         ///< Main cache storage typedef
    typedef typename List::iterator List_Iter;                ///< Main cache iterator
    typedef typename List::const_iterator List_cIter;         ///< Main cache iterator (const)
    typedef std::vector< Key > Key_List;                      ///< List of keys
    typedef typename Key_List::iterator Key_List_Iter;        ///< Main cache iterator
    typedef typename Key_List::const_iterator Key_List_cIter; ///< Main cache iterator (const)
    typedef google::sparse_hash_map< Key, List_Iter > Map;    ///< Index typedef
    typedef std::pair< Key, List_Iter > Pair;                 ///< Pair of Map elements
    typedef typename Map::iterator Map_Iter;			      ///< Index iterator
    typedef typename Map::const_iterator Map_cIter;           ///< Index iterator (const)

private:
    List _list;               ///< Main cache storage
    Map _index;               ///< Cache storage index
    unsigned long _max_size;  ///< Maximum abstract size of the cache
    unsigned long _curr_size; ///< Current abstract size of the cache

public:
    LRUCache( const unsigned long Size ) : _max_size( Size ), _curr_size( 0 ) {
        _index.set_deleted_key(UINT_MAX);
    }

    ~LRUCache() { clear(); }

    inline const unsigned long size( void ) const { return _curr_size; }

    inline const unsigned long max_size( void ) const { return _max_size; }

    /// Clears all storage and indices.
    void clear( void ) {
        _list.clear();
        _index.clear();
    };

    inline bool exists( const Key &key ) const {
        return _index.find( key ) != _index.end();
    }

    inline void remove( const Key &key ) {
        Map_Iter miter = _index.find( key );
        if( miter == _index.end() ) return;
        _remove( miter );
    }

    inline void touch( const Key &key ) {
        _touch( key );
    }

    inline Data *fetch_ptr( const Key &key, bool touch = true ) {
        Map_Iter miter = _index.find( key );
        if( miter == _index.end() ) return NULL;
        _touch( key );
        return &(miter->second->second);
    }

    inline bool fetch( const Key &key, Data &data, bool touch_data = true ) {
        Map_Iter miter = _index.find( key );
        if( miter == _index.end() ) return false;
        if( touch_data )
            _touch( key );
        data = miter->second->second;
        return true;
    }

    inline void insert( const Key &key, const Data data ) {
        // Touch the key, if it exists, then replace the content.
        Map_Iter miter = _touch( key );
        if( miter != _index.end() )
            _remove( miter );

        // Ok, do the actual insert at the head of the list
        _list.push_front( std::make_pair( key, data ) );
        List_Iter liter = _list.begin();

        // Store the index
        _index.insert( std::make_pair( key, liter ) );
        _curr_size += Sizefn()( data );

        // Check to see if we need to remove an element due to exceeding max_size
        while( _curr_size > _max_size ) {
            std::cout << "removing element " << std::endl;
            // Remove the last element.
            liter = _list.end();
            --liter;
            _remove( liter->first );
        }
    }

    inline const Key_List get_all_keys( void ) {
        Key_List ret;
        for( List_cIter liter = _list.begin(); liter != _list.end(); liter++ )
            ret.push_back( liter->first );
        return ret;
    }

private:
    inline Map_Iter _touch( const Key &key ) {
        Map_Iter miter = _index.find( key );
        if( miter == _index.end() ) return miter;
        // Move the found node to the head of the list.
        _list.splice( _list.begin(), _list, miter->second );
        return miter;
    }

    inline void _remove( const Map_Iter &miter ) {
        _curr_size -= Sizefn()( miter->second->second );
        _list.erase( miter->second );
        _index.erase( miter );
    }

    inline void _remove( const Key &key ) {
        Map_Iter miter = _index.find( key );
        _remove( miter );
    }
};

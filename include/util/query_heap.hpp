#ifndef OSRM_UTIL_QUERY_HEAP_HPP
#define OSRM_UTIL_QUERY_HEAP_HPP

#include <boost/assert.hpp>
#include <boost/heap/d_ary_heap.hpp>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <unordered_map>
#include <vector>

namespace osrm::util
{

template <typename NodeID, typename Key> class ArrayStorage
{
  public:
    explicit ArrayStorage(std::size_t size) : positions(size, 0) {}

    Key &operator[](NodeID node) { return positions[node]; }

    Key peek_index(const NodeID node) const { return positions[node]; }

    void Clear() {}

  private:
    std::vector<Key> positions;
};

template <typename NodeID, typename Key> class UnorderedMapStorage
{
  public:
    explicit UnorderedMapStorage(std::size_t) { nodes.rehash(1000); }

    Key &operator[](const NodeID node) { return nodes[node]; }

    Key peek_index(const NodeID node) const
    {
        const auto iter = nodes.find(node);
        if (std::end(nodes) != iter)
        {
            return iter->second;
        }
        return std::numeric_limits<Key>::max();
    }

    Key const &operator[](const NodeID node) const
    {
        auto iter = nodes.find(node);
        return iter->second;
    }

    void Clear() { nodes.clear(); }

  private:
    std::unordered_map<NodeID, Key> nodes;
};

template <typename NodeID,
          typename Key,
          template <typename N, typename K> class BaseIndexStorage = UnorderedMapStorage,
          template <typename N, typename K> class OverlayIndexStorage = ArrayStorage>
class TwoLevelStorage
{
  public:
    explicit TwoLevelStorage(std::size_t number_of_nodes, std::size_t number_of_overlay_nodes)
        : number_of_overlay_nodes(number_of_overlay_nodes), base(number_of_nodes),
          overlay(number_of_overlay_nodes)
    {
    }

    Key &operator[](const NodeID node)
    {
        if (node < number_of_overlay_nodes)
        {
            return overlay[node];
        }
        else
        {
            return base[node];
        }
    }

    Key peek_index(const NodeID node) const
    {
        if (node < number_of_overlay_nodes)
        {
            return overlay.peek_index(node);
        }
        else
        {
            return base.peek_index(node);
        }
    }

    Key const &operator[](const NodeID node) const
    {
        if (node < number_of_overlay_nodes)
        {
            return overlay[node];
        }
        else
        {
            return base[node];
        }
    }

    void Clear()
    {
        base.Clear();
        overlay.Clear();
    }

  private:
    const std::size_t number_of_overlay_nodes;
    BaseIndexStorage<NodeID, Key> base;
    OverlayIndexStorage<NodeID, Key> overlay;
};

template <typename NodeID,
          typename Key,
          typename Weight,
          typename Data,
          typename IndexStorage = ArrayStorage<NodeID, NodeID>>
class QueryHeap
{
  private:
    struct HeapData
    {
        Weight weight;
        Key index;

        bool operator>(const HeapData &other) const
        {
            if (weight == other.weight)
            {
                return index > other.index;
            }
            return weight > other.weight;
        }
    };
    using HeapContainer = boost::heap::d_ary_heap<HeapData,
                                                  boost::heap::arity<4>,
                                                  boost::heap::mutable_<true>,
                                                  boost::heap::compare<std::greater<HeapData>>>;
    using HeapHandle = typename HeapContainer::handle_type;

  public:
    using WeightType = Weight;
    using DataType = Data;

    struct HeapNode
    {
        HeapHandle handle;
        NodeID node;
        Weight weight;
        Data data;
    };

    template <typename... StorageArgs> explicit QueryHeap(StorageArgs... args) : node_index(args...)
    {
        Clear();
    }

    void Clear()
    {
        heap.clear();
        inserted_nodes.clear();
        node_index.Clear();
    }

    std::size_t Size() const { return heap.size(); }

    bool Empty() const { return 0 == Size(); }

    void Insert(NodeID node, Weight weight, const Data &data)
    {
        BOOST_ASSERT(node < std::numeric_limits<NodeID>::max());
        const auto index = static_cast<Key>(inserted_nodes.size());
        const auto handle = heap.emplace(HeapData{weight, index});
        inserted_nodes.emplace_back(HeapNode{handle, node, weight, data});
        node_index[node] = index;
    }

    Data &GetData(NodeID node)
    {
        const auto index = node_index.peek_index(node);
        BOOST_ASSERT((int)index >= 0 && (int)index < (int)inserted_nodes.size());
        return inserted_nodes[index].data;
    }

    HeapNode &getHeapNode(NodeID node)
    {
        const auto index = node_index.peek_index(node);
        BOOST_ASSERT((int)index >= 0 && (int)index < (int)inserted_nodes.size());
        return inserted_nodes[index];
    }

    Data const &GetData(NodeID node) const
    {
        const auto index = node_index.peek_index(node);
        BOOST_ASSERT((int)index >= 0 && (int)index < (int)inserted_nodes.size());
        return inserted_nodes[index].data;
    }

    const Weight &GetKey(NodeID node) const
    {
        const auto index = node_index.peek_index(node);
        return inserted_nodes[index].weight;
    }

    bool WasRemoved(const NodeID node) const
    {
        BOOST_ASSERT(WasInserted(node));
        const Key index = node_index.peek_index(node);

        // Use end iterator as a reliable "non-existent" handle.
        // Default-constructed handles are singular and
        // can only be checked-compared to another singular instance.
        // Behaviour investigated at https://lists.boost.org/boost-users/2017/08/87787.php,
        // eventually confirmation at https://stackoverflow.com/a/45622940/151641.
        // Corrected in https://github.com/Project-OSRM/osrm-backend/pull/4396
        auto const end_it = const_cast<HeapContainer &>(heap).end();  // non-const iterator
        auto const none_handle = heap.s_handle_from_iterator(end_it); // from non-const iterator
        return inserted_nodes[index].handle == none_handle;
    }

    bool WasInserted(const NodeID node) const
    {
        const auto index = node_index.peek_index(node);
        if (index >= static_cast<decltype(index)>(inserted_nodes.size()))
        {
            return false;
        }
        return inserted_nodes[index].node == node;
    }

    HeapNode *GetHeapNodeIfWasInserted(const NodeID node)
    {
        const auto index = node_index.peek_index(node);
        if (index >= static_cast<decltype(index)>(inserted_nodes.size()) ||
            inserted_nodes[index].node != node)
        {
            return nullptr;
        }
        return &inserted_nodes[index];
    }

    const HeapNode *GetHeapNodeIfWasInserted(const NodeID node) const
    {
        const auto index = node_index.peek_index(node);
        if (index >= static_cast<decltype(index)>(inserted_nodes.size()) ||
            inserted_nodes[index].node != node)
        {
            return nullptr;
        }
        return &inserted_nodes[index];
    }

    NodeID Min() const
    {
        BOOST_ASSERT(!heap.empty());
        return inserted_nodes[heap.top().index].node;
    }

    Weight MinKey() const
    {
        BOOST_ASSERT(!heap.empty());
        return heap.top().weight;
    }

    NodeID DeleteMin()
    {
        BOOST_ASSERT(!heap.empty());
        const Key removedIndex = heap.top().index;
        heap.pop();
        inserted_nodes[removedIndex].handle = heap.s_handle_from_iterator(heap.end());
        return inserted_nodes[removedIndex].node;
    }

    HeapNode &DeleteMinGetHeapNode()
    {
        BOOST_ASSERT(!heap.empty());
        const Key removedIndex = heap.top().index;
        heap.pop();
        inserted_nodes[removedIndex].handle = heap.s_handle_from_iterator(heap.end());
        return inserted_nodes[removedIndex];
    }

    void DeleteAll()
    {
        auto const none_handle = heap.s_handle_from_iterator(heap.end());
        std::for_each(inserted_nodes.begin(),
                      inserted_nodes.end(),
                      [&none_handle](auto &node) { node.handle = none_handle; });
        heap.clear();
    }

    void DecreaseKey(NodeID node, Weight weight)
    {
        BOOST_ASSERT(!WasRemoved(node));
        const auto index = node_index.peek_index(node);
        auto &reference = inserted_nodes[index];
        reference.weight = weight;
        heap.increase(reference.handle, HeapData{weight, static_cast<Key>(index)});
    }

    void DecreaseKey(const HeapNode &heapNode)
    {
        BOOST_ASSERT(!WasRemoved(heapNode.node));
        heap.increase(heapNode.handle, HeapData{heapNode.weight, (*heapNode.handle).index});
    }

  private:
    std::vector<HeapNode> inserted_nodes;
    HeapContainer heap;
    IndexStorage node_index;
};
} // namespace osrm::util

#endif // OSRM_UTIL_QUERY_HEAP_HPP

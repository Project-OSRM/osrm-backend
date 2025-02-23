#ifndef OSRM_UTIL_QUERY_HEAP_HPP
#define OSRM_UTIL_QUERY_HEAP_HPP

#include <boost/assert.hpp>
#include <boost/heap/d_ary_heap.hpp>

#include "d_ary_heap.hpp"
#include <algorithm>
#include <limits>
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

        bool operator<(const HeapData &other) const
        {
            if (weight == other.weight)
            {
                return index < other.index;
            }
            return weight < other.weight;
        }
    };
    using HeapContainer = DAryHeap<HeapData, 4>;
    using HeapHandle = typename HeapContainer::HeapHandle;

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
        checkInvariants();

        BOOST_ASSERT(node < std::numeric_limits<NodeID>::max());
        const auto index = static_cast<Key>(inserted_nodes.size());
        inserted_nodes.emplace_back(HeapNode{heap.size(), node, weight, data});

        heap.emplace(HeapData{weight, index},
                     [this](const auto &heapData, auto new_handle)
                     { inserted_nodes[heapData.index].handle = new_handle; });
        node_index[node] = index;

        checkInvariants();
    }

    void checkInvariants()
    {
#ifndef NDEBUG
        for (size_t handle = 0; handle < heap.size(); ++handle)
        {
            auto &in_heap = heap[handle];
            auto &inserted = inserted_nodes[in_heap.index];
            BOOST_ASSERT(in_heap.weight == inserted.weight);
            BOOST_ASSERT(inserted.handle == handle);
        }
#endif // !NDEBUG
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
        return inserted_nodes[index].handle == HeapContainer::INVALID_HANDLE;
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
        inserted_nodes[removedIndex].handle = HeapContainer::INVALID_HANDLE;

        heap.pop([this](const auto &heapData, auto new_handle)
                 { inserted_nodes[heapData.index].handle = new_handle; });
        return inserted_nodes[removedIndex].node;
    }

    HeapNode &DeleteMinGetHeapNode()
    {
        BOOST_ASSERT(!heap.empty());
        checkInvariants();
        const Key removedIndex = heap.top().index;
        inserted_nodes[removedIndex].handle = HeapContainer::INVALID_HANDLE;
        heap.pop([this](const auto &heapData, auto new_handle)
                 { inserted_nodes[heapData.index].handle = new_handle; });
        checkInvariants();
        return inserted_nodes[removedIndex];
    }

    void DeleteAll()
    {
        std::for_each(inserted_nodes.begin(),
                      inserted_nodes.end(),
                      [&](auto &node) { node.handle = HeapContainer::INVALID_HANDLE; });
        heap.clear();
    }

    void DecreaseKey(NodeID node, Weight weight)
    {
        BOOST_ASSERT(!WasRemoved(node));
        const auto index = node_index.peek_index(node);
        auto &reference = inserted_nodes[index];
        reference.weight = weight;
        heap.decrease(reference.handle,
                      HeapData{weight, static_cast<Key>(index)},
                      [this](const auto &heapData, auto new_handle)
                      { inserted_nodes[heapData.index].handle = new_handle; });
    }

    void DecreaseKey(const HeapNode &heapNode)
    {
        BOOST_ASSERT(!WasRemoved(heapNode.node));
        heap.decrease(heapNode.handle,
                      HeapData{heapNode.weight, heap[heapNode.handle].index},
                      [this](const auto &heapData, auto new_handle)
                      { inserted_nodes[heapData.index].handle = new_handle; });
    }

  private:
    std::vector<HeapNode> inserted_nodes;
    HeapContainer heap;
    IndexStorage node_index;
};

} // namespace osrm::util

#endif // OSRM_UTIL_QUERY_HEAP_HPP

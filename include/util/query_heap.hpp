#ifndef OSRM_UTIL_QUERY_HEAP_HPP
#define OSRM_UTIL_QUERY_HEAP_HPP

#include <boost/assert.hpp>
#include <boost/heap/d_ary_heap.hpp>

#include <algorithm>
#include <limits>
#include <map>
#include <unordered_map>
#include <vector>

namespace osrm
{
namespace util
{

template <typename NodeID, typename Key> class GenerationArrayStorage
{
    using GenerationCounter = std::uint16_t;

  public:
    explicit GenerationArrayStorage(std::size_t size)
        : positions(size, 0), generation(1), generations(size, 0)
    {
    }

    Key &operator[](NodeID node)
    {
        generation[node] = generation;
        return positions[node];
    }

    Key peek_index(const NodeID node) const
    {
        if (generations[node] < generation)
        {
            return std::numeric_limits<Key>::max();
        }
        return positions[node];
    }

    void Clear()
    {
        generation++;
        // if generation overflows we end up at 0 again and need to clear the vector
        if (generation == 0)
        {
            generation = 1;
            std::fill(generations.begin(), generations.end(), 0);
        }
    }

  private:
    GenerationCounter generation;
    std::vector<GenerationCounter> generations;
    std::vector<Key> positions;
};

template <typename NodeID, typename Key> class ArrayStorage
{
  public:
    explicit ArrayStorage(std::size_t size) : positions(size, 0) {}

    ~ArrayStorage() {}

    Key &operator[](NodeID node) { return positions[node]; }

    Key peek_index(const NodeID node) const { return positions[node]; }

    void Clear() {}

  private:
    std::vector<Key> positions;
};

template <typename NodeID, typename Key> class MapStorage
{
  public:
    explicit MapStorage(std::size_t) {}

    Key &operator[](NodeID node) { return nodes[node]; }

    void Clear() { nodes.clear(); }

    Key peek_index(const NodeID node) const
    {
        const auto iter = nodes.find(node);
        if (nodes.end() != iter)
        {
            return iter->second;
        }
        return std::numeric_limits<Key>::max();
    }

  private:
    std::map<NodeID, Key> nodes;
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
          typename Weight,
          typename Data,
          typename IndexStorage = ArrayStorage<NodeID, NodeID>>
class QueryHeap
{
  public:
    using WeightType = Weight;
    using DataType = Data;

    explicit QueryHeap(std::size_t maxID) : node_index(maxID) { Clear(); }

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
        const auto index = static_cast<Key>(inserted_nodes.size());
        const auto handle = heap.push(std::make_pair(weight, index));
        inserted_nodes.emplace_back(HeapNode{handle, node, weight, data});
        node_index[node] = index;
    }

    void InsertVisited(NodeID node, Weight weight, const Data &data)
    {
        const auto index = static_cast<Key>(inserted_nodes.size());
        inserted_nodes.emplace_back(HeapNode{HeapHandle{}, node, weight, data});
        node_index[node] = index;
    }

    Data &GetData(NodeID node)
    {
        const auto index = node_index.peek_index(node);
        return inserted_nodes[index].data;
    }

    Data const &GetData(NodeID node) const
    {
        const auto index = node_index.peek_index(node);
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
        return inserted_nodes[index].handle == HeapHandle{};
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

    NodeID Min() const
    {
        BOOST_ASSERT(!heap.empty());
        return inserted_nodes[heap.top().second].node;
    }

    Weight MinKey() const
    {
        BOOST_ASSERT(!heap.empty());
        return heap.top().first;
    }

    NodeID DeleteMin()
    {
        BOOST_ASSERT(!heap.empty());
        const Key removedIndex = heap.top().second;
        heap.pop();
        inserted_nodes[removedIndex].handle = HeapHandle{};
        return inserted_nodes[removedIndex].node;
    }

    void DeleteAll()
    {
        std::for_each(inserted_nodes.begin(), inserted_nodes.end(), [](auto &node) {
            node.handle = HeapHandle();
        });
        heap.clear();
    }

    void DecreaseKey(NodeID node, Weight weight)
    {
        BOOST_ASSERT(!WasRemoved(node));
        const auto index = node_index.peek_index(node);
        auto &reference = inserted_nodes[index];
        reference.weight = weight;
        heap.increase(reference.handle, std::make_pair(weight, index));
    }

  private:
    using HeapData = std::pair<Weight, Key>;
    using HeapContainer = boost::heap::d_ary_heap<HeapData,
                                                  boost::heap::arity<4>,
                                                  boost::heap::mutable_<true>,
                                                  boost::heap::compare<std::greater<HeapData>>>;
    using HeapHandle = typename HeapContainer::handle_type;

    struct HeapNode
    {
        HeapHandle handle;
        NodeID node;
        Weight weight;
        Data data;
    };

    std::vector<HeapNode> inserted_nodes;
    HeapContainer heap;
    IndexStorage node_index;
};
}
}

#endif // OSRM_UTIL_QUERY_HEAP_HPP

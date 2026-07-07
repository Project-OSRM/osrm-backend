#ifndef OSRM_UTIL_BROWSE_RESISTANT_CACHE_HPP
#define OSRM_UTIL_BROWSE_RESISTANT_CACHE_HPP

#include <cstddef>
#include <functional>
#include <list>
#include <unordered_map>

namespace osrm::util
{

// Two-tier LRU cache resistant to sequential scan (browse) pollution.
//
// New entries land in L1 (probationary). A second access promotes to L2 (protected).
// L2 evictions demote back to L1. L1 evictions are removed entirely.
// This prevents one-shot sequential accesses from evicting frequently-used entries.
//
// CostFn: callable taking const Value& and returning size_t (memory cost in bytes).
template <typename Key,
          typename Value,
          typename CostFn,
          typename Hash = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>>
class BrowseResistantCache
{
    struct Entry
    {
        Key key;
        Value value;
        size_t memory_cost;
    };

    using List = std::list<Entry>;
    using ListIterator = List::iterator;
    using Map = std::unordered_map<Key, ListIterator, Hash, KeyEqual>;

  public:
    BrowseResistantCache(size_t l1_budget_bytes, size_t l2_budget_bytes, CostFn cost_fn)
        : l1_budget_(l1_budget_bytes), l2_budget_(l2_budget_bytes), l1_used_(0), l2_used_(0),
          cost_fn_(std::move(cost_fn))
    {
    }

    Value *get(const Key &key)
    {
        if (auto it = l2_map_.find(key); it != l2_map_.end())
        {
            l2_list_.splice(l2_list_.begin(), l2_list_, it->second);
            return &it->second->value;
        }

        if (auto it = l1_map_.find(key); it != l1_map_.end())
        {
            promote(it->second);
            // After promote()+evict_l2(), the entry may have been demoted back to L1
            // if its cost exceeds the L2 budget. Re-find in both tiers.
            if (auto l2_it = l2_map_.find(key); l2_it != l2_map_.end())
                return &l2_it->second->value;
            if (auto l1_it = l1_map_.find(key); l1_it != l1_map_.end())
                return &l1_it->second->value;
        }

        return nullptr;
    }

    void insert(const Key &key, Value value)
    {
        // Remove any existing entry for this key to keep map/list consistent.
        if (auto it = l2_map_.find(key); it != l2_map_.end())
        {
            l2_used_ -= it->second->memory_cost;
            l2_list_.erase(it->second);
            l2_map_.erase(it);
        }
        else if (auto it = l1_map_.find(key); it != l1_map_.end())
        {
            l1_used_ -= it->second->memory_cost;
            l1_list_.erase(it->second);
            l1_map_.erase(it);
        }

        size_t cost = cost_fn_(value);
        l1_list_.emplace_front(Entry{key, std::move(value), cost});
        l1_map_.emplace(key, l1_list_.begin());
        l1_used_ += cost;
        evict_l1();
    }

    void clear()
    {
        l1_list_.clear();
        l1_map_.clear();
        l2_list_.clear();
        l2_map_.clear();
        l1_used_ = 0;
        l2_used_ = 0;
    }

    size_t l1_size() const { return l1_map_.size(); }
    size_t l2_size() const { return l2_map_.size(); }
    size_t size() const { return l1_size() + l2_size(); }
    size_t l1_memory_used() const { return l1_used_; }
    size_t l2_memory_used() const { return l2_used_; }

  private:
    void promote(ListIterator l1_it)
    {
        size_t cost = l1_it->memory_cost;
        l1_used_ -= cost;
        l1_map_.erase(l1_it->key);

        l2_list_.splice(l2_list_.begin(), l1_list_, l1_it);
        l2_map_.emplace(l2_list_.front().key, l2_list_.begin());
        l2_used_ += cost;

        evict_l2();
    }

    void demote(ListIterator l2_it)
    {
        size_t cost = l2_it->memory_cost;
        l2_used_ -= cost;
        l2_map_.erase(l2_it->key);

        l1_list_.splice(l1_list_.begin(), l2_list_, l2_it);
        l1_map_.emplace(l1_list_.front().key, l1_list_.begin());
        l1_used_ += cost;
    }

    void evict_l1()
    {
        while (l1_used_ > l1_budget_ && !l1_list_.empty())
        {
            auto &victim = l1_list_.back();
            l1_used_ -= victim.memory_cost;
            l1_map_.erase(victim.key);
            l1_list_.pop_back();
        }
    }

    void evict_l2()
    {
        while (l2_used_ > l2_budget_ && !l2_list_.empty())
        {
            auto victim_it = std::prev(l2_list_.end());
            demote(victim_it);
        }
        evict_l1();
    }

    size_t l1_budget_;
    size_t l2_budget_;
    size_t l1_used_;
    size_t l2_used_;
    CostFn cost_fn_;

    List l1_list_;
    Map l1_map_;
    List l2_list_;
    Map l2_map_;
};

} // namespace osrm::util

#endif // OSRM_UTIL_BROWSE_RESISTANT_CACHE_HPP

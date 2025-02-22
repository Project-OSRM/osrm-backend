#ifndef STD_HASH_HPP
#define STD_HASH_HPP

#include <cstddef>
#include <functional>
#include <tuple>
#include <utility>
#include <vector>

// this is largely inspired by boost's hash combine as can be found in
// "The C++ Standard Library" 2nd Edition. Nicolai M. Josuttis. 2012.

template <typename T> void hash_combine(std::size_t &seed, const T &val)
{
    seed ^= std::hash<T>()(val) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template <typename It> void hash_range(std::size_t &seed, It first, const It last)
{
    for (; first != last; ++first)
    {
        hash_combine(seed, *first);
    }
}

template <typename T> void hash_val(std::size_t &seed, const T &val) { hash_combine(seed, val); }

template <typename T, typename... Types>
void hash_val(std::size_t &seed, const T &val, const Types &...args)
{
    hash_combine(seed, val);
    hash_val(seed, args...);
}

template <typename... Types> std::size_t hash_val(const Types &...args)
{
    std::size_t seed = 0;
    hash_val(seed, args...);
    return seed;
}

namespace std
{
template <typename... T> struct hash<std::tuple<T...>>
{
    template <std::size_t... I>
    static auto apply_tuple(const std::tuple<T...> &t, std::index_sequence<I...>)
    {
        std::size_t seed = 0;
        return ((seed = hash_val(std::get<I>(t), seed)), ...);
    }

    auto operator()(const std::tuple<T...> &t) const
    {
        return apply_tuple(t, std::make_index_sequence<sizeof...(T)>());
    }
};

template <typename T1, typename T2> struct hash<std::pair<T1, T2>>
{
    std::size_t operator()(const std::pair<T1, T2> &pair) const
    {
        return hash_val(pair.first, pair.second);
    }
};

template <typename T> struct hash<std::vector<T>>
{
    auto operator()(const std::vector<T> &lane_description) const
    {
        std::size_t seed = 0;
        hash_range(seed, lane_description.begin(), lane_description.end());
        return seed;
    }
};

} // namespace std

#endif // STD_HASH_HPP

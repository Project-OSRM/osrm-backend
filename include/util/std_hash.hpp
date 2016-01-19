#ifndef STD_HASH_HPP
#define STD_HASH_HPP

#include <functional>

// this is largely inspired by boost's hash combine as can be found in
// "The C++ Standard Library" 2nd Edition. Nicolai M. Josuttis. 2012.

template <typename T> void hash_combine(std::size_t &seed, const T &val)
{
    seed ^= std::hash<T>()(val) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template <typename T> void hash_val(std::size_t &seed, const T &val) { hash_combine(seed, val); }

template <typename T, typename... Types>
void hash_val(std::size_t &seed, const T &val, const Types &... args)
{
    hash_combine(seed, val);
    hash_val(seed, args...);
}

template <typename... Types> std::size_t hash_val(const Types &... args)
{
    std::size_t seed = 0;
    hash_val(seed, args...);
    return seed;
}

namespace std
{
template <typename T1, typename T2> struct hash<std::pair<T1, T2>>
{
    size_t operator()(const std::pair<T1, T2> &pair) const
    {
        return hash_val(pair.first, pair.second);
    }
};
}

#endif // STD_HASH_HPP

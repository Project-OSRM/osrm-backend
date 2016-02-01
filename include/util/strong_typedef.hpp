#ifndef STRONG_TYPEDEF_HPP
#define STRONG_TYPEDEF_HPP

#include <iostream>
#include <type_traits>
#include <functional>

namespace osrm
{

/* Creates strongly typed wrappers around scalar types.
 * Useful for stopping accidental assignment of lats to lons,
 * etc.  Also clarifies what this random "int" value is
 * being used for.
 */
#define OSRM_STRONG_TYPEDEF(From, To)                                                              \
    class To final                                                                                 \
    {                                                                                              \
        static_assert(std::is_arithmetic<From>(), "");                                             \
        From x;                                                                                    \
        friend std::ostream& operator<<(std::ostream& stream, const To& inst);                     \
                                                                                                   \
      public:                                                                                      \
        To() = default;                                                                            \
        explicit To(const From x_) : x(x_) {}                                                      \
        explicit operator From &() { return x; }                                                   \
        explicit operator const From &() const { return x; }                                       \
        bool operator<(const To &z_) const { return x < static_cast<const From>(z_); }             \
        bool operator>(const To &z_) const { return x > static_cast<const From>(z_); }             \
        bool operator<=(const To &z_) const { return x <= static_cast<const From>(z_); }           \
        bool operator>=(const To &z_) const { return x >= static_cast<const From>(z_); }           \
        bool operator==(const To &z_) const { return x == static_cast<const From>(z_); }           \
        bool operator!=(const To &z_) const { return x != static_cast<const From>(z_); }           \
    };                                                                                             \
    inline From To##_to_##From(To to) { return static_cast<From>(to); }                            \
    namespace std                                                                                  \
    {                                                                                              \
    template <> struct hash<To>                                                                    \
    {                                                                                              \
        std::size_t operator()(const To &k) const                                                  \
        {                                                                                          \
            return std::hash<From>()(static_cast<const From>(k));                                  \
        }                                                                                          \
    };                                                                                             \
    }                                                                                              \
    inline std::ostream& operator<<(std::ostream& stream, const To& inst) {                        \
        return stream << #To << '(' << inst.x << ')';                                              \
    }
}

#endif // OSRM_STRONG_TYPEDEF_HPP

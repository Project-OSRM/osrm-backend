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
        explicit operator From () const { return x; }                                        \
        To operator+(const To rhs_) const { return To(x + static_cast<const From>(rhs_)); }        \
        To operator-(const To rhs_) const { return To(x - static_cast<const From>(rhs_)); }        \
        To operator*(const To rhs_) const { return To(x * static_cast<const From>(rhs_)); }        \
        To operator/(const To rhs_) const { return To(x / static_cast<const From>(rhs_)); }        \
        bool operator<(const To z_) const { return x < static_cast<const From>(z_); }              \
        bool operator>(const To z_) const { return x > static_cast<const From>(z_); }              \
        bool operator<=(const To z_) const { return x <= static_cast<const From>(z_); }            \
        bool operator>=(const To z_) const { return x >= static_cast<const From>(z_); }            \
        bool operator==(const To z_) const { return x == static_cast<const From>(z_); }            \
        bool operator!=(const To z_) const { return x != static_cast<const From>(z_); }            \
    };                                                                                             \
    inline std::ostream& operator<<(std::ostream& stream, const To& inst) {                        \
        return stream << #To << '(' << inst.x << ')';                                              \
    }

#define OSRM_STRONG_TYPEDEF_HASHABLE(From, To)                                                     \
    namespace std                                                                                  \
    {                                                                                              \
    template <> struct hash<To>                                                                    \
    {                                                                                              \
        std::size_t operator()(const To &k) const                                                  \
        {                                                                                          \
            return std::hash<From>()(static_cast<const From>(k));                                  \
        }                                                                                          \
    };                                                                                             \
    }

}

#endif // OSRM_STRONG_TYPEDEF_HPP

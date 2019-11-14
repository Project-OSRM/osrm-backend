#pragma once

namespace mapbox {
namespace geometry {

struct empty
{
}; //  this Geometry type represents the empty point set, ∅, for the coordinate space (OGC Simple Features).

constexpr bool operator==(empty, empty) { return true; }
constexpr bool operator!=(empty, empty) { return false; }
constexpr bool operator<(empty, empty) { return false; }
constexpr bool operator>(empty, empty) { return false; }
constexpr bool operator<=(empty, empty) { return true; }
constexpr bool operator>=(empty, empty) { return true; }

} // namespace geometry
} // namespace mapbox

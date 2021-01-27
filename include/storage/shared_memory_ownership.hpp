#ifndef SHARED_MEMORY_OWNERSHIP_HPP
#define SHARED_MEMORY_OWNERSHIP_HPP

namespace osrm
{
namespace storage
{

enum class Ownership
{
    Container,
    View,
    External
};
}
} // namespace osrm

#endif // SHARED_MEMORY_OWNERSHIP_HPP

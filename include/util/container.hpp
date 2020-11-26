#ifndef CONTAINER_HPP
#define CONTAINER_HPP

#include <utility>

namespace osrm
{
namespace util
{

template <class Container> void append_to_container(Container &&) {}

template <class Container, typename T, typename... Args>
void append_to_container(Container &&container, T value, Args &&... args)
{
    container.emplace_back(value);
    append_to_container(std::forward<Container>(container), std::forward<Args>(args)...);
}
} // namespace util
} // namespace osrm

#endif

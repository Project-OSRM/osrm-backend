#ifndef HEADER_HPP
#define HEADER_HPP

#include <algorithm>
#include <string>

namespace osrm::server::http
{

struct header
{
    // explicitly use default copy c'tor as adding move c'tor
    header &operator=(const header &other) = default;
    header(std::string name, std::string value) : name(std::move(name)), value(std::move(value)) {}
    header(header &&other) noexcept : name(std::move(other.name)), value(std::move(other.value)) {}

    void clear()
    {
        name.clear();
        value.clear();
    }

    std::string name;
    std::string value;
};
} // namespace osrm::server::http

#endif // HEADER_HPP

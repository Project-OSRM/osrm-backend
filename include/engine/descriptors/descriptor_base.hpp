#ifndef DESCRIPTOR_BASE_HPP
#define DESCRIPTOR_BASE_HPP

#include "util/coordinate_calculation.hpp"
#include "engine/internal_route_result.hpp"
#include "engine/phantom_node.hpp"
#include "util/typedefs.hpp"

#include <boost/assert.hpp>

#include "osrm/json_container.hpp"

#include <string>
#include <unordered_map>
#include <vector>

struct DescriptorTable : public std::unordered_map<std::string, unsigned>
{
    unsigned get_id(const std::string &key)
    {
        auto iter = find(key);
        if (iter != end())
        {
            return iter->second;
        }
        return 0;
    }
};

struct DescriptorConfig
{
    DescriptorConfig() : instructions(true), geometry(true), encode_geometry(true), zoom_level(18)
    {
    }

    template <class OtherT>
    DescriptorConfig(const OtherT &other)
        : instructions(other.print_instructions), geometry(other.geometry),
          encode_geometry(other.compression), zoom_level(other.zoom_level)
    {
        BOOST_ASSERT(zoom_level >= 0);
    }

    bool instructions;
    bool geometry;
    bool encode_geometry;
    short zoom_level;
};

template <class DataFacadeT> class BaseDescriptor
{
  public:
    BaseDescriptor() {}
    // Maybe someone can explain the pure virtual destructor thing to me (dennis)
    virtual ~BaseDescriptor() {}
    virtual void Run(const InternalRouteResult &raw_route, osrm::json::Object &json_result) = 0;
    virtual void SetConfig(const DescriptorConfig &c) = 0;
};

#endif // DESCRIPTOR_BASE_HPP

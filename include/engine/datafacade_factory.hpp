#ifndef OSRM_ENGINE_DATAFACADE_FACTORY_HPP
#define OSRM_ENGINE_DATAFACADE_FACTORY_HPP

#include "extractor/class_data.hpp"
#include "extractor/profile_properties.hpp"

#include "engine/api/base_parameters.hpp"
#include "engine/api/tile_parameters.hpp"

#include "util/integer_range.hpp"

#include "storage/shared_datatype.hpp"

#include <array>
#include <memory>
#include <unordered_map>

namespace osrm
{
namespace engine
{
// This class selects the right facade for
template <typename FacadeT> class DataFacadeFactory
{
  public:
    DataFacadeFactory() = default;

    template <typename AllocatorT> DataFacadeFactory(std::shared_ptr<AllocatorT> allocator)
    {
        for (const auto index : util::irange<std::size_t>(0, facades.size()))
        {
            facades[index] = std::make_shared<FacadeT>(allocator, index);
        }

        properties = allocator->GetLayout().template GetBlockPtr<extractor::ProfileProperties>(
            allocator->GetMemory(), storage::DataLayout::PROPERTIES);

        for (const auto index : util::irange<std::size_t>(0, properties->class_names.size()))
        {
            const std::string name = properties->GetClassName(index);
            if (!name.empty())
            {
                name_to_class[name] = extractor::getClassData(index);
            }
        }
    }

    std::shared_ptr<FacadeT> Get(const api::TileParameters &) const
    {
        return facades[0];
    }

    std::shared_ptr<FacadeT> Get(const api::BaseParameters &params) const
    {
        if (params.avoid.empty())
            return facades[0];

        extractor::ClassData mask = 0;
        for (const auto &name : params.avoid)
        {
            auto class_mask_iter = name_to_class.find(name);
            if (class_mask_iter != name_to_class.end())
            {
                mask |= class_mask_iter->second;
            }
        }

        auto avoid_iter = std::find(
            properties->avoidable_classes.begin(), properties->avoidable_classes.end(), mask);
        if (avoid_iter != properties->avoidable_classes.end())
        {
            auto avoid_index = std::distance(properties->avoidable_classes.begin(), avoid_iter);
            return facades[avoid_index];
        }

        // FIXME We need proper error handling here
        return {};
    }

  private:
    std::array<std::shared_ptr<FacadeT>, extractor::MAX_AVOIDABLE_CLASSES> facades;
    std::unordered_map<std::string, extractor::ClassData> name_to_class;
    const extractor::ProfileProperties *properties = nullptr;
};
}
}

#endif

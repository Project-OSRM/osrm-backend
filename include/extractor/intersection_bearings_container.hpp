#ifndef OSRM_EXTRACTOR_BEARING_CONTAINER_HPP
#define OSRM_EXTRACTOR_BEARING_CONTAINER_HPP

#include "storage/shared_memory_ownership.hpp"
#include "storage/tar_fwd.hpp"

#include "util/guidance/bearing_class.hpp"
#include "util/range_table.hpp"
#include "util/vector_view.hpp"

#include <limits>
#include <numeric>

namespace osrm::extractor
{
namespace detail
{
template <storage::Ownership Ownership> class IntersectionBearingsContainer;
} // namespace detail

namespace serialization
{
template <storage::Ownership Ownership>
void read(storage::tar::FileReader &reader,
          const std::string &name,
          detail::IntersectionBearingsContainer<Ownership> &turn_data);

template <storage::Ownership Ownership>
void write(storage::tar::FileWriter &writer,
           const std::string &name,
           const detail::IntersectionBearingsContainer<Ownership> &turn_data);
} // namespace serialization

namespace detail
{
template <storage::Ownership Ownership> class IntersectionBearingsContainer
{
    template <typename T> using Vector = util::ViewOrVector<T, Ownership>;
    template <unsigned size> using RangeTable = util::RangeTable<size, Ownership>;

  public:
    IntersectionBearingsContainer() = default;
    IntersectionBearingsContainer(IntersectionBearingsContainer &&) = default;
    IntersectionBearingsContainer(const IntersectionBearingsContainer &) = default;
    IntersectionBearingsContainer &operator=(IntersectionBearingsContainer &&) = default;
    IntersectionBearingsContainer &operator=(const IntersectionBearingsContainer &) = default;

    // NOLINTNEXTLINE(performance-unnecessary-value-param)
    IntersectionBearingsContainer(std::vector<BearingClassID> node_to_class_id,
                                  const std::vector<util::guidance::BearingClass> &bearing_classes)
        : node_to_class_id_(std::move(node_to_class_id))
    {
        std::vector<unsigned> bearing_counts(bearing_classes.size());
        std::transform(bearing_classes.begin(),
                       bearing_classes.end(),
                       bearing_counts.begin(),
                       [](const auto &bearings) { return bearings.getAvailableBearings().size(); });
        // NOLINTNEXTLINE(bugprone-fold-init-type)
        auto total_bearings = std::accumulate(bearing_counts.begin(), bearing_counts.end(), 0);
        class_id_to_ranges_table_ = RangeTable<16>{bearing_counts};

        values_.reserve(total_bearings);
        for (const auto &bearing_class : bearing_classes)
        {
            const auto &bearings = bearing_class.getAvailableBearings();
            values_.insert(values_.end(), bearings.begin(), bearings.end());
        }
    }

    IntersectionBearingsContainer(Vector<DiscreteBearing> values,
                                  Vector<BearingClassID> node_to_class_id,
                                  RangeTable<16> class_id_to_ranges_table)
        : values_(std::move(values)), node_to_class_id_(std::move(node_to_class_id)),
          class_id_to_ranges_table_(std::move(class_id_to_ranges_table))
    {
    }

    IntersectionBearingsContainer(Vector<DiscreteBearing> values,
                                  Vector<std::uint16_t> packed_classes,
                                  RangeTable<16> ranges)
        requires(Ownership == storage::Ownership::View)
        : values_(std::move(values)), class_id_to_ranges_table_(std::move(ranges)),
          packed_classes_(std::move(packed_classes))
    {
    }

    // Returns the bearing class for an intersection node
    util::guidance::BearingClass GetBearingClass(const NodeID node) const
    {
        const auto class_id =
            packed_classes_.empty()
                ? node_to_class_id_[node]
                : (packed_classes_[node] == 65535 ? std::numeric_limits<BearingClassID>::max()
                                                  : BearingClassID(packed_classes_[node]));
        auto range = class_id_to_ranges_table_.GetRange(class_id);
        util::guidance::BearingClass result;
        std::for_each(values_.begin() + range.front(),
                      values_.begin() + range.back() + 1,
                      [&](const DiscreteBearing &bearing) { result.add(bearing); });
        return result;
    }

    friend void serialization::read<Ownership>(storage::tar::FileReader &reader,
                                               const std::string &name,
                                               IntersectionBearingsContainer &turn_data_container);
    friend void
    serialization::write<Ownership>(storage::tar::FileWriter &writer,
                                    const std::string &name,
                                    const IntersectionBearingsContainer &turn_data_container);

  private:
    Vector<DiscreteBearing> values_;
    Vector<BearingClassID> node_to_class_id_;
    RangeTable<16> class_id_to_ranges_table_;
    Vector<std::uint16_t> packed_classes_;
};
} // namespace detail

using IntersectionBearingsContainer =
    detail::IntersectionBearingsContainer<storage::Ownership::Container>;
using IntersectionBearingsView = detail::IntersectionBearingsContainer<storage::Ownership::View>;
} // namespace osrm::extractor

#endif

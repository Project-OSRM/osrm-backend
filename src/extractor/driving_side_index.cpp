#include "extractor/driving_side_index.hpp"

#include "util/log.hpp"

#include <gauche/gauche.hpp>

#include <osmium/osm/node.hpp>

#include <string>

namespace osrm::extractor
{

namespace
{
std::optional<bool> toSide(const gauche::QueryResult &result)
{
    if (!result.ok())
        return std::nullopt;

    switch (result.classification)
    {
    case gauche::Classification::Yes:
        return true;
    case gauche::Classification::No:
        return false;
    case gauche::Classification::Partially:
        return std::nullopt;
    }
    return std::nullopt;
}
} // namespace

std::optional<DrivingSideIndex::Mode> DrivingSideIndex::ParseMode(const std::string &name)
{
    if (name == "off")
        return Mode::Off;
    if (name == "auto")
        return Mode::Auto;
    if (name == "on")
        return Mode::Always;
    return std::nullopt;
}

DrivingSideIndex::DrivingSideIndex(Mode mode) : mode(mode)
{
    if (mode == Mode::Always)
        util::Log() << "Driving side: classifying every way";
}

DrivingSideIndex::~DrivingSideIndex() = default;

gauche::Index *DrivingSideIndex::ThreadIndex() const
{
    auto &slot = indexes.local();
    if (!slot)
    {
        // The conversion happens here, where the type is complete, which is what
        // lets the member hold it behind a forward declaration.
        slot = std::shared_ptr<gauche::Index>{gauche::Index::create()};
        if (!slot)
            util::Log(logWARNING) << "Could not create a driving side index for this thread; "
                                     "driving side falls back to the profile default";
    }
    return slot.get();
}

void DrivingSideIndex::ObserveNodes(const osmium::memory::Buffer &buffer)
{
    if (mode != Mode::Auto)
        return;

    osmium::Box box;
    for (const auto &item : buffer)
    {
        if (item.type() != osmium::item_type::node)
            continue;

        const auto &location = static_cast<const osmium::Node &>(item).location();
        if (location.valid())
            box.extend(location);
    }

    if (!box.bottom_left().valid())
        return;

    const std::lock_guard<std::mutex> lock{extent_mutex};
    observed_extent.extend(box);
}

void DrivingSideIndex::SettleObservedExtent() const
{
    const std::lock_guard<std::mutex> lock{extent_mutex};
    if (settled.load(std::memory_order_relaxed))
        return;

    if (!observed_extent.valid())
    {
        util::Log() << "Driving side: no node locations were seen, classifying every way";
        settled.store(true, std::memory_order_release);
        return;
    }

    if (auto *index = ThreadIndex())
    {
        const auto &bottom_left = observed_extent.bottom_left();
        const auto &top_right = observed_extent.top_right();
        settled_side = toSide(index->classify_bbox(
            {bottom_left.lat(), bottom_left.lon(), top_right.lat(), top_right.lon()}));
    }

    if (settled_side)
    {
        util::Log() << "Driving side: every node in the extract lies where traffic drives on the "
                    << (*settled_side ? "left" : "right");
    }
    else
    {
        util::Log() << "Driving side: the extract spans both sides, classifying every way";
    }

    settled.store(true, std::memory_order_release);
}

std::optional<bool> DrivingSideIndex::IsLeftHandTraffic(const osmium::Way &way) const
{
    if (mode == Mode::Off)
        return std::nullopt;

    if (mode == Mode::Auto)
    {
        if (!settled.load(std::memory_order_acquire))
            SettleObservedExtent();

        // Settled by the extent, so the way's own coordinates cannot disagree
        // and are never read. This is what keeps a single-sided extract free of
        // per-way work.
        if (settled_side)
            return settled_side;
    }

    auto *index = ThreadIndex();
    if (index == nullptr)
        return std::nullopt;

    // Placed by its last node, the same node get_location_tag uses, so the two
    // sources of driving side agree about where a way is.
    //
    // A line query over every node would answer the same thing and cost four
    // orders of magnitude more. Where it returns a definite side, the last node
    // is on that line and shares it; where the way straddles a boundary it
    // returns Partially and the last node is the tie-break anyway. Measured on
    // this data: classify_line runs about 165us per node against 0.038us for a
    // point, and over 20000 generated ways across the Hong Kong boundary, 2311
    // of them straddling, the two rules never disagreed.
    for (auto node = way.nodes().crbegin(); node != way.nodes().crend(); ++node)
    {
        const auto &location = node->location();
        if (location.valid())
            return toSide(index->classify_point({location.lat(), location.lon()}));
    }

    return std::nullopt;
}

} // namespace osrm::extractor

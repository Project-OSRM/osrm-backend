#include "extractor/driving_side_index.hpp"

#include "util/log.hpp"

#include <gauche/gauche.hpp>

#include <string>
#include <vector>

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

DrivingSideIndex::DrivingSideIndex(Mode mode) : mode(mode) {}

DrivingSideIndex::~DrivingSideIndex() = default;

gauche::Index *DrivingSideIndex::ThreadIndex() const
{
    auto &slot = indexes.local();
    if (!slot)
    {
        slot = gauche::Index::create();
        if (!slot)
            util::Log(logWARNING) << "Could not create a driving side index for this thread; "
                                     "driving side falls back to the profile default";
    }
    return slot.get();
}

std::optional<bool> DrivingSideIndex::SettleExtent(const osmium::Box &box)
{
    if (mode == Mode::Off)
        return std::nullopt;

    if (mode == Mode::Always)
    {
        needs_way_lookups = true;
        util::Log() << "Driving side: classifying every way";
        return std::nullopt;
    }

    auto *index = ThreadIndex();
    if (index == nullptr)
        return std::nullopt;

    if (!box.valid())
    {
        needs_way_lookups = true;
        util::Log() << "Driving side: the input declares no bounding box, so every way is "
                       "classified. Node locations are kept for the whole parse, which costs "
                       "time and memory; an input with a bounding box avoids both.";
        return std::nullopt;
    }

    const auto &bottom_left = box.bottom_left();
    const auto &top_right = box.top_right();
    settled_side = toSide(index->classify_bbox(
        {bottom_left.lat(), bottom_left.lon(), top_right.lat(), top_right.lon()}));

    if (settled_side)
    {
        util::Log() << "Driving side: the whole extract drives on the "
                    << (*settled_side ? "left" : "right");
    }
    else
    {
        needs_way_lookups = true;
        util::Log() << "Driving side: the extract spans both sides, classifying every way";
    }

    return settled_side;
}

std::optional<bool> DrivingSideIndex::IsLeftHandTraffic(const osmium::Way &way) const
{
    if (mode == Mode::Off)
        return std::nullopt;

    // Settled by the bounding box, so the way's own coordinates cannot disagree
    // and are never read. This is what keeps a single-country extract free of
    // per-way work.
    if (settled_side)
        return settled_side;

    auto *index = ThreadIndex();
    if (index == nullptr)
        return std::nullopt;

    std::vector<gauche::Point> line;
    line.reserve(way.nodes().size());
    for (const auto &node : way.nodes())
    {
        const auto &location = node.location();
        if (location.valid())
            line.push_back({location.lat(), location.lon()});
    }

    if (line.empty())
        return std::nullopt;

    const auto side = toSide(index->classify_line(line));
    if (side)
        return side;

    // The way straddles a boundary. Settle it on its last node, which is the
    // same node get_location_tag uses to place a way, so the two sources of
    // driving side agree about where a way is.
    return toSide(index->classify_point(line.back()));
}

} // namespace osrm::extractor

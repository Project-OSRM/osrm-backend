#include "engine/hint.hpp"
#include "engine/base64.hpp"
#include "engine/datafacade/datafacade_base.hpp"

#include <boost/assert.hpp>

#include <algorithm>
#include <iterator>
#include <tuple>
#include <unordered_set>

namespace osrm::engine
{

bool SegmentHint::IsValid(const util::Coordinate new_input_coordinates,
                          const datafacade::BaseDataFacade &facade) const
{
    auto is_same_input_coordinate = new_input_coordinates.lon == phantom.input_location.lon &&
                                    new_input_coordinates.lat == phantom.input_location.lat;
    // FIXME this does not use the number of nodes to validate the phantom because
    // GetNumberOfNodes()
    // depends on the graph which is algorithm dependent
    return is_same_input_coordinate && phantom.IsValid() && facade.GetCheckSum() == data_checksum;
}

std::string SegmentHint::ToBase64() const
{
    auto base64 = encodeBase64Bytewise(*this);

    // Make safe for usage as GET parameter in URLs
    std::replace(begin(base64), end(base64), '+', '-');
    std::replace(begin(base64), end(base64), '/', '_');

    return base64;
}

SegmentHint SegmentHint::FromBase64(const std::string &base64Hint)
{
    BOOST_ASSERT_MSG(base64Hint.size() == ENCODED_SEGMENT_HINT_SIZE, "Hint has invalid size");

    // We need mutability but don't want to change the API
    auto encoded = base64Hint;

    // Reverses above encoding we need for GET parameters in URL
    std::replace(begin(encoded), end(encoded), '-', '+');
    std::replace(begin(encoded), end(encoded), '_', '/');

    return decodeBase64Bytewise<SegmentHint>(encoded);
}

bool operator==(const SegmentHint &lhs, const SegmentHint &rhs)
{
    return std::tie(lhs.phantom, lhs.data_checksum) == std::tie(rhs.phantom, rhs.data_checksum);
}

bool operator!=(const SegmentHint &lhs, const SegmentHint &rhs) { return !(lhs == rhs); }

std::ostream &operator<<(std::ostream &out, const SegmentHint &hint)
{
    return out << hint.ToBase64();
}

std::string Hint::ToBase64() const
{
    std::string res;
    for (const auto &hint : segment_hints)
    {
        res += hint.ToBase64();
    }
    return res;
}

Hint Hint::FromBase64(const std::string &base64Hint)
{

    BOOST_ASSERT_MSG(base64Hint.size() % ENCODED_SEGMENT_HINT_SIZE == 0,
                     "SegmentHint has invalid size");

    auto num_hints = base64Hint.size() / ENCODED_SEGMENT_HINT_SIZE;
    std::vector<SegmentHint> res(num_hints);

    for (const auto i : util::irange<std::size_t>(0UL, num_hints))
    {
        auto start_offset = i * ENCODED_SEGMENT_HINT_SIZE;
        auto end_offset = start_offset + ENCODED_SEGMENT_HINT_SIZE;
        res[i] = SegmentHint::FromBase64(
            std::string(base64Hint.begin() + start_offset, base64Hint.begin() + end_offset));
    }

    return {std::move(res)};
}

bool Hint::IsValid(const util::Coordinate new_input_coordinates,
                   const datafacade::BaseDataFacade &facade) const
{
    const auto all_valid = std::all_of(segment_hints.begin(),
                                       segment_hints.end(),
                                       [&](const auto &seg_hint)
                                       { return seg_hint.IsValid(new_input_coordinates, facade); });
    if (!all_valid)
    {
        return false;
    }

    // Check hints do not contain duplicate segment pairs
    // We can't allow duplicates as search heaps do not support it.
    std::unordered_set<NodeID> forward_segments;
    std::unordered_set<NodeID> reverse_segments;
    for (const auto &seg_hint : segment_hints)
    {
        const auto forward_res = forward_segments.insert(seg_hint.phantom.forward_segment_id.id);
        if (!forward_res.second)
        {
            return false;
        }
        const auto backward_res = reverse_segments.insert(seg_hint.phantom.reverse_segment_id.id);
        if (!backward_res.second)
        {
            return false;
        }
    }
    return true;
}

} // namespace osrm::engine

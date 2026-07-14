#include "engine/hint.hpp"
#include "engine/base64.hpp"
#include "engine/datafacade/datafacade_base.hpp"

#include <boost/assert.hpp>

#include <algorithm>
#include <iterator>
#include <string>

namespace osrm::engine
{

bool SegmentHint::IsValid(const util::Coordinate /*new_input_coordinates*/,
                          const datafacade::BaseDataFacade &facade) const
{
    // Hints are deprecated; only validate via dataset checksum to avoid rejecting
    // requests that still include hints from older responses.
    return facade.GetCheckSum() == data_checksum;
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
{ return lhs.deprecated_data == rhs.deprecated_data && lhs.data_checksum == rhs.data_checksum; }

bool operator!=(const SegmentHint &lhs, const SegmentHint &rhs) { return !(lhs == rhs); }

std::ostream &operator<<(std::ostream &out, const SegmentHint &hint)
{ return out << hint.ToBase64(); }

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

bool Hint::IsValid(const util::Coordinate /*new_input_coordinates*/,
                   const datafacade::BaseDataFacade &facade) const
{
    // Hints are deprecated; validate each segment hint via checksum-only check.
    const auto all_valid =
        std::all_of(segment_hints.begin(),
                    segment_hints.end(),
                    [&](const auto &seg_hint) { return seg_hint.IsValid({}, facade); });
    return all_valid;
}

} // namespace osrm::engine

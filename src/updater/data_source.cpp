#include "updater/data_source.hpp"

#include "updater/csv_file_parser.hpp"
#include "updater/parquet_file_parser.hpp"
#include "updater/source.hpp"

#include <boost/fusion/adapted/std_pair.hpp>
#include <boost/fusion/include/adapt_adt.hpp>

// clang-format off
BOOST_FUSION_ADAPT_STRUCT(osrm::updater::Segment,
                         (decltype(osrm::updater::Segment::from), from)
                         (decltype(osrm::updater::Segment::to), to))
BOOST_FUSION_ADAPT_STRUCT(osrm::updater::SpeedSource,
                          (decltype(osrm::updater::SpeedSource::speed), speed)
                          (decltype(osrm::updater::SpeedSource::rate), rate))
BOOST_FUSION_ADAPT_STRUCT(osrm::updater::Turn,
                          (decltype(osrm::updater::Turn::from), from)
                          (decltype(osrm::updater::Turn::via), via)
                          (decltype(osrm::updater::Turn::to), to))
BOOST_FUSION_ADAPT_STRUCT(osrm::updater::PenaltySource,
                          (decltype(osrm::updater::PenaltySource::duration), duration)
                          (decltype(osrm::updater::PenaltySource::weight), weight))
// clang-format on
namespace
{
namespace qi = boost::spirit::qi;
}

namespace osrm
{
namespace updater
{
namespace data
{

namespace
{
std::unique_ptr<FilesParser<Segment, SpeedSource>>
makeSegmentParser(SpeedAndTurnPenaltyFormat format)
{
    switch (format)
    {
    case SpeedAndTurnPenaltyFormat::CSV:
    {
        static const auto value_if_blank = std::numeric_limits<double>::quiet_NaN();
        const qi::real_parser<double, qi::ureal_policies<double>> unsigned_double;
        return std::make_unique<CSVFilesParser<Segment, SpeedSource>>(
            qi::ulong_long >> ',' >> qi::ulong_long,
            unsigned_double >> -(',' >> (qi::double_ | qi::attr(value_if_blank))));
    }
    case SpeedAndTurnPenaltyFormat::PARQUET:
        return std::make_unique<ParquetFilesParser<Segment, SpeedSource>>();
    }
    BOOST_UNREACHABLE_RETURN({});
}

std::unique_ptr<FilesParser<Turn, PenaltySource>> makeTurnParser(SpeedAndTurnPenaltyFormat format)
{
    switch (format)
    {
    case SpeedAndTurnPenaltyFormat::CSV:
    {
        return std::make_unique<CSVFilesParser<Turn, PenaltySource>>(
            qi::ulong_long >> ',' >> qi::ulong_long >> ',' >> qi::ulong_long,
            qi::double_ >> -(',' >> qi::double_));
    }
    case SpeedAndTurnPenaltyFormat::PARQUET:
        return std::make_unique<ParquetFilesParser<Turn, PenaltySource>>();
    }
    BOOST_UNREACHABLE_RETURN({});
}

} // namespace

SegmentLookupTable readSegmentValues(const std::vector<std::string> &paths,
                                     SpeedAndTurnPenaltyFormat format)
{
    auto parser = makeSegmentParser(format);

    // Check consistency of keys in the result lookup table
    auto result = (*parser)(paths);
    const auto found_inconsistency =
        std::find_if(std::begin(result.lookup), std::end(result.lookup), [](const auto &entry) {
            return entry.first.from == entry.first.to;
        });
    if (found_inconsistency != std::end(result.lookup))
    {
        util::Log(logWARNING) << "Empty segment in CSV with node " +
                                     std::to_string(found_inconsistency->first.from);
    }

    return result;
}

TurnLookupTable readTurnValues(const std::vector<std::string> &paths,
                               SpeedAndTurnPenaltyFormat format)
{
    auto parser = makeTurnParser(format);
    return (*parser)(paths);
}
} // namespace data
} // namespace updater
} // namespace osrm

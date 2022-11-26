#include <boost/test/tools/old/interface.hpp>
#include <updater/data_source.hpp>
#include <boost/test/unit_test.hpp>

using namespace osrm;
using namespace osrm::updater;

BOOST_AUTO_TEST_CASE(parquet_readSegmentValues)
{
    boost::filesystem::path test_path(TEST_DATA_DIR "/speeds_file.parquet");
    SegmentLookupTable segment_lookup_table = data::readSegmentValues({test_path.string()}, SpeedAndTurnPenaltyFormat::PARQUET);
    BOOST_CHECK_EQUAL(segment_lookup_table.lookup.size(), 2);
}
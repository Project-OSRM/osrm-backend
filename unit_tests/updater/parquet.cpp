#include <arrow/array.h>
#include <arrow/io/file.h>
#include <arrow/table.h>
#include <arrow/type_fwd.h>
#include <boost/test/tools/old/interface.hpp>
#include <boost/test/unit_test.hpp>
#include <parquet/stream_writer.h>
#include <updater/data_source.hpp>

using namespace osrm;
using namespace osrm::updater;

BOOST_AUTO_TEST_CASE(parquet_readSegmentValues)
{
    {
        SegmentLookupTable segment_lookup_table = data::readSegmentValues(
            {boost::filesystem::path{TEST_DATA_DIR "/speed.parquet"}.string()},
            SpeedAndTurnPenaltyFormat::PARQUET);
        BOOST_CHECK_EQUAL(segment_lookup_table.lookup.size(), 100);
    }

    {
        SegmentLookupTable segment_lookup_table = data::readSegmentValues(
            {boost::filesystem::path{TEST_DATA_DIR "/speed_without_rate.parquet"}.string()},
            SpeedAndTurnPenaltyFormat::PARQUET);
        BOOST_CHECK_EQUAL(segment_lookup_table.lookup.size(), 100);
    }
}
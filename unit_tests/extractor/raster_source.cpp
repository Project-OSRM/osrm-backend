#include "extractor/raster_source.hpp"
#include "util/exception.hpp"
#include "util/typedefs.hpp"

#include <osrm/coordinate.hpp>

#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(raster_source)

using namespace osrm;
using namespace osrm::extractor;

int normalize(double coord) { return static_cast<int>(coord * COORDINATE_PRECISION); }

#define CHECK_QUERY(source_id, lon, lat, expected)                                                 \
    BOOST_CHECK_EQUAL(sources.GetRasterDataFromSource(source_id, lon, lat).datum, expected)

#define CHECK_INTERPOLATE(source_id, lon, lat, expected)                                           \
    BOOST_CHECK_EQUAL(sources.GetRasterInterpolateFromSource(source_id, lon, lat).datum, expected)

BOOST_AUTO_TEST_CASE(raster_test)
{
    SourceContainer sources;
    int source_id = sources.LoadRasterSource(
        "../unit_tests/fixtures/raster_data.asc", 0, 0.09, 0, 0.09, 10, 10);
    BOOST_CHECK_EQUAL(source_id, 0);

    // Expected nearest-neighbor queries
    //     EDGES
    CHECK_QUERY(0, 0.00, 0.00, 10);
    CHECK_QUERY(0, 0.00, 0.09, 10);
    CHECK_QUERY(0, 0.09, 0.00, 40);
    CHECK_QUERY(0, 0.09, 0.09, 100);
    CHECK_QUERY(0, 0.09, 0.07, 140);
    //     OUT OF BOUNDS
    CHECK_QUERY(0, -0.1, 0.07, RasterDatum::get_invalid());
    CHECK_QUERY(0, -0.1, -3.0, RasterDatum::get_invalid());
    CHECK_QUERY(0, 0.3, 23.0, RasterDatum::get_invalid());
    //     ARBITRARY - AT DATA
    CHECK_QUERY(0, 0.06, 0.06, 100);
    CHECK_QUERY(0, 0.08, 0.05, 160);
    CHECK_QUERY(0, 0.01, 0.05, 20);
    //     ARBITRARY - BETWEEN DATA
    CHECK_QUERY(0, 0.054, 0.023, 40);
    CHECK_QUERY(0, 0.056, 0.028, 80);
    CHECK_QUERY(0, 0.05, 0.028, 60);

    // Expected bilinear interpolation queries
    //     EDGES - same as above
    CHECK_INTERPOLATE(0, 0.00, 0.00, 10);
    CHECK_INTERPOLATE(0, 0.00, 0.09, 10);
    CHECK_INTERPOLATE(0, 0.09, 0.00, 40);
    CHECK_INTERPOLATE(0, 0.09, 0.09, 100);
    CHECK_INTERPOLATE(0, 0.09, 0.07, 140);
    //     OUT OF BOUNDS - same as above
    CHECK_INTERPOLATE(0, -0.1, 0.07, RasterDatum::get_invalid());
    CHECK_INTERPOLATE(0, -0.1, -3.0, RasterDatum::get_invalid());
    CHECK_INTERPOLATE(0, 0.3, 23.0, RasterDatum::get_invalid());
    //     ARBITRARY - AT DATA - same as above
    CHECK_INTERPOLATE(0, 0.06, 0.06, 100);
    CHECK_INTERPOLATE(0, 0.08, 0.05, 160);
    CHECK_INTERPOLATE(0, 0.01, 0.05, 20);
    //     ARBITRARY - BETWEEN DATA
    CHECK_INTERPOLATE(0, 0.054, 0.023, 54);
    CHECK_INTERPOLATE(0, 0.056, 0.028, 68);
    CHECK_INTERPOLATE(0, 0.05, 0.028, 56);

    int source_already_loaded_id = sources.LoadRasterSource(
        "../unit_tests/fixtures/raster_data.asc", 0, 0.09, 0, 0.09, 10, 10);

    BOOST_CHECK_EQUAL(source_already_loaded_id, 0);
    BOOST_CHECK_THROW(sources.GetRasterDataFromSource(1, normalize(0.02), normalize(0.02)),
                      util::exception);

    BOOST_CHECK_THROW(
        sources.LoadRasterSource("../unit_tests/fixtures/nonexistent.asc", 0, 0.1, 0, 0.1, 7, 7),
        util::exception);
}

BOOST_AUTO_TEST_SUITE_END()

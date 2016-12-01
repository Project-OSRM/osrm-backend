#include "extractor/raster_source.hpp"

#include "util/exception.hpp"
#include "util/exception_utils.hpp"
#include "util/log.hpp"
#include "util/timing_util.hpp"
#include "util/typedefs.hpp"

#include <cmath>

namespace osrm
{
namespace extractor
{

RasterSource::RasterSource(RasterGrid _raster_data,
                           std::size_t _width,
                           std::size_t _height,
                           int _xmin,
                           int _xmax,
                           int _ymin,
                           int _ymax)
    : xstep(CalcSize(_xmin, _xmax, _width)), ystep(CalcSize(_ymin, _ymax, _height)),
      raster_data(std::move(_raster_data)), width(_width), height(_height), xmin(_xmin),
      xmax(_xmax), ymin(_ymin), ymax(_ymax)
{
    BOOST_ASSERT(xstep != 0);
    BOOST_ASSERT(ystep != 0);
}

float RasterSource::CalcSize(int min, int max, std::size_t count) const
{
    BOOST_ASSERT(count > 0);
    return (max - min) / (static_cast<float>(count) - 1);
}

// Query raster source for nearest data point
RasterDatum RasterSource::GetRasterData(const int lon, const int lat) const
{
    if (lon < xmin || lon > xmax || lat < ymin || lat > ymax)
    {
        return {};
    }

    const std::size_t xth = static_cast<std::size_t>(round((lon - xmin) / xstep));
    const std::size_t yth = static_cast<std::size_t>(round((ymax - lat) / ystep));

    return {raster_data(xth, yth)};
}

// Query raster source using bilinear interpolation
RasterDatum RasterSource::GetRasterInterpolate(const int lon, const int lat) const
{
    if (lon < xmin || lon > xmax || lat < ymin || lat > ymax)
    {
        return {};
    }

    const auto xthP = (lon - xmin) / xstep;
    const auto ythP =
        (ymax - lat) /
        ystep; // the raster texture uses a different coordinate system with y pointing downwards

    const std::size_t top = static_cast<std::size_t>(fmax(floor(ythP), 0));
    const std::size_t bottom = static_cast<std::size_t>(fmin(ceil(ythP), height - 1));
    const std::size_t left = static_cast<std::size_t>(fmax(floor(xthP), 0));
    const std::size_t right = static_cast<std::size_t>(fmin(ceil(xthP), width - 1));

    // Calculate distances from corners for bilinear interpolation
    const float fromLeft = xthP - left; // this is the fraction part of xthP
    const float fromTop = ythP - top;   // this is the fraction part of ythP
    const float fromRight = 1 - fromLeft;
    const float fromBottom = 1 - fromTop;

    return {static_cast<std::int32_t>(raster_data(left, top) * (fromRight * fromBottom) +
                                      raster_data(right, top) * (fromLeft * fromBottom) +
                                      raster_data(left, bottom) * (fromRight * fromTop) +
                                      raster_data(right, bottom) * (fromLeft * fromTop))};
}

// Load raster source into memory
int SourceContainer::LoadRasterSource(const std::string &path_string,
                                      double xmin,
                                      double xmax,
                                      double ymin,
                                      double ymax,
                                      std::size_t nrows,
                                      std::size_t ncols)
{
    const auto _xmin = static_cast<std::int32_t>(util::toFixed(util::FloatLongitude{xmin}));
    const auto _xmax = static_cast<std::int32_t>(util::toFixed(util::FloatLongitude{xmax}));
    const auto _ymin = static_cast<std::int32_t>(util::toFixed(util::FloatLatitude{ymin}));
    const auto _ymax = static_cast<std::int32_t>(util::toFixed(util::FloatLatitude{ymax}));

    const auto itr = LoadedSourcePaths.find(path_string);
    if (itr != LoadedSourcePaths.end())
    {
        util::Log() << "[source loader] Already loaded source '" << path_string << "' at source_id "
                    << itr->second;
        return itr->second;
    }

    int source_id = static_cast<int>(LoadedSources.size());

    util::Log() << "[source loader] Loading from " << path_string << "  ... ";
    TIMER_START(loading_source);

    boost::filesystem::path filepath(path_string);
    if (!boost::filesystem::exists(filepath))
    {
        throw util::exception(path_string + " does not exist" + SOURCE_REF);
    }

    RasterGrid rasterData{filepath, ncols, nrows};

    RasterSource source{std::move(rasterData), ncols, nrows, _xmin, _xmax, _ymin, _ymax};
    TIMER_STOP(loading_source);
    LoadedSourcePaths.emplace(path_string, source_id);
    LoadedSources.push_back(std::move(source));

    util::Log() << "[source loader] ok, after " << TIMER_SEC(loading_source) << "s";

    return source_id;
}

// External function for looking up nearest data point from a specified source
RasterDatum SourceContainer::GetRasterDataFromSource(unsigned int source_id, double lon, double lat)
{
    if (LoadedSources.size() < source_id + 1)
    {
        throw util::exception("Attempted to access source " + std::to_string(source_id) +
                              ", but there are only " + std::to_string(LoadedSources.size()) +
                              " loaded" + SOURCE_REF);
    }

    BOOST_ASSERT(lat < 90);
    BOOST_ASSERT(lat > -90);
    BOOST_ASSERT(lon < 180);
    BOOST_ASSERT(lon > -180);

    const auto &found = LoadedSources[source_id];
    return found.GetRasterData(static_cast<std::int32_t>(util::toFixed(util::FloatLongitude{lon})),
                               static_cast<std::int32_t>(util::toFixed(util::FloatLatitude{lat})));
}

// External function for looking up interpolated data from a specified source
RasterDatum
SourceContainer::GetRasterInterpolateFromSource(unsigned int source_id, double lon, double lat)
{
    if (LoadedSources.size() < source_id + 1)
    {
        throw util::exception("Attempted to access source " + std::to_string(source_id) +
                              ", but there are only " + std::to_string(LoadedSources.size()) +
                              " loaded" + SOURCE_REF);
    }

    BOOST_ASSERT(lat < 90);
    BOOST_ASSERT(lat > -90);
    BOOST_ASSERT(lon < 180);
    BOOST_ASSERT(lon > -180);

    const auto &found = LoadedSources[source_id];
    return found.GetRasterInterpolate(
        static_cast<std::int32_t>(util::toFixed(util::FloatLongitude{lon})),
        static_cast<std::int32_t>(util::toFixed(util::FloatLatitude{lat})));
}
}
}

/*

Copyright (c) 2015, Project OSRM contributors
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "extractor/raster_source.hpp"

#include "util/simple_logger.hpp"
#include "util/timing_util.hpp"

#include "osrm/coordinate.hpp"

#include <cmath>

RasterSource::RasterSource(RasterGrid _raster_data,
                           std::size_t _width,
                           std::size_t _height,
                           int _xmin,
                           int _xmax,
                           int _ymin,
                           int _ymax)
    : xstep(calcSize(_xmin, _xmax, _width)), ystep(calcSize(_ymin, _ymax, _height)),
      raster_data(std::move(_raster_data)), width(_width), height(_height), xmin(_xmin),
      xmax(_xmax), ymin(_ymin), ymax(_ymax)
{
    BOOST_ASSERT(xstep != 0);
    BOOST_ASSERT(ystep != 0);
}

float RasterSource::calcSize(int min, int max, std::size_t count) const
{
    BOOST_ASSERT(count > 0);
    return (max - min) / (static_cast<float>(count) - 1);
}

// Query raster source for nearest data point
RasterDatum RasterSource::getRasterData(const int lon, const int lat) const
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
RasterDatum RasterSource::getRasterInterpolate(const int lon, const int lat) const
{
    if (lon < xmin || lon > xmax || lat < ymin || lat > ymax)
    {
        return {};
    }

    const auto xthP = (lon - xmin) / xstep;
    const auto ythP = (ymax - lat) / ystep;

    const std::size_t top = static_cast<std::size_t>(fmax(floor(ythP), 0));
    const std::size_t bottom = static_cast<std::size_t>(fmin(ceil(ythP), height - 1));
    const std::size_t left = static_cast<std::size_t>(fmax(floor(xthP), 0));
    const std::size_t right = static_cast<std::size_t>(fmin(ceil(xthP), width - 1));

    // Calculate distances from corners for bilinear interpolation
    const float fromLeft = (lon - left * xstep + xmin) / xstep;
    const float fromTop = (ymax - top * ystep - lat) / ystep;
    const float fromRight = 1 - fromLeft;
    const float fromBottom = 1 - fromTop;

    return {static_cast<std::int32_t>(raster_data(left, top) * (fromRight * fromBottom) +
                                      raster_data(right, top) * (fromLeft * fromBottom) +
                                      raster_data(left, bottom) * (fromRight * fromTop) +
                                      raster_data(right, bottom) * (fromLeft * fromTop))};
}

// Load raster source into memory
int SourceContainer::loadRasterSource(const std::string &path_string,
                                      double xmin,
                                      double xmax,
                                      double ymin,
                                      double ymax,
                                      std::size_t nrows,
                                      std::size_t ncols)
{
    const auto _xmin = static_cast<int>(xmin * COORDINATE_PRECISION);
    const auto _xmax = static_cast<int>(xmax * COORDINATE_PRECISION);
    const auto _ymin = static_cast<int>(ymin * COORDINATE_PRECISION);
    const auto _ymax = static_cast<int>(ymax * COORDINATE_PRECISION);

    const auto itr = LoadedSourcePaths.find(path_string);
    if (itr != LoadedSourcePaths.end())
    {
        SimpleLogger().Write() << "[source loader] Already loaded source '" << path_string
                               << "' at source_id " << itr->second;
        return itr->second;
    }

    int source_id = static_cast<int>(LoadedSources.size());

    SimpleLogger().Write() << "[source loader] Loading from " << path_string << "  ... ";
    TIMER_START(loading_source);

    boost::filesystem::path filepath(path_string);
    if (!boost::filesystem::exists(filepath))
    {
        throw osrm::exception("error reading: no such path");
    }

    RasterGrid rasterData{filepath, ncols, nrows};

    RasterSource source{std::move(rasterData), ncols, nrows, _xmin, _xmax, _ymin, _ymax};
    TIMER_STOP(loading_source);
    LoadedSourcePaths.emplace(path_string, source_id);
    LoadedSources.push_back(std::move(source));

    SimpleLogger().Write() << "[source loader] ok, after " << TIMER_SEC(loading_source) << "s";

    return source_id;
}

// External function for looking up nearest data point from a specified source
RasterDatum SourceContainer::getRasterDataFromSource(unsigned int source_id, int lon, int lat)
{
    if (LoadedSources.size() < source_id + 1)
    {
        throw osrm::exception("error reading: no such loaded source");
    }

    BOOST_ASSERT(lat < (90 * COORDINATE_PRECISION));
    BOOST_ASSERT(lat > (-90 * COORDINATE_PRECISION));
    BOOST_ASSERT(lon < (180 * COORDINATE_PRECISION));
    BOOST_ASSERT(lon > (-180 * COORDINATE_PRECISION));

    const auto &found = LoadedSources[source_id];
    return found.getRasterData(lon, lat);
}

// External function for looking up interpolated data from a specified source
RasterDatum
SourceContainer::getRasterInterpolateFromSource(unsigned int source_id, int lon, int lat)
{
    if (LoadedSources.size() < source_id + 1)
    {
        throw osrm::exception("error reading: no such loaded source");
    }

    BOOST_ASSERT(lat < (90 * COORDINATE_PRECISION));
    BOOST_ASSERT(lat > (-90 * COORDINATE_PRECISION));
    BOOST_ASSERT(lon < (180 * COORDINATE_PRECISION));
    BOOST_ASSERT(lon > (-180 * COORDINATE_PRECISION));

    const auto &found = LoadedSources[source_id];
    return found.getRasterInterpolate(lon, lat);
}

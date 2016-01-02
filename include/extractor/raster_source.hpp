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

#ifndef RASTER_SOURCE_HPP
#define RASTER_SOURCE_HPP

#include "util/osrm_exception.hpp"

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/spirit/include/qi_int.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/assert.hpp>

#include <unordered_map>
#include <iterator>

/**
    \brief Small wrapper around raster source queries to optionally provide results
    gracefully, depending on source bounds
*/
struct RasterDatum
{
    static std::int32_t get_invalid() { return std::numeric_limits<std::int32_t>::max(); }

    std::int32_t datum = get_invalid();

    RasterDatum() = default;

    RasterDatum(std::int32_t _datum) : datum(_datum) {}
};

class RasterGrid
{
  public:
    RasterGrid(const boost::filesystem::path &filepath, std::size_t _xdim, std::size_t _ydim)
    {
        xdim = _xdim;
        ydim = _ydim;
        _data.reserve(ydim * xdim);

        boost::filesystem::ifstream stream(filepath);
        if (!stream)
        {
            throw osrm::exception("Unable to open raster file.");
        }

        stream.seekg(0, std::ios_base::end);
        std::string buffer;
        buffer.resize(static_cast<std::size_t>(stream.tellg()));

        stream.seekg(0, std::ios_base::beg);

        BOOST_ASSERT(buffer.size() > 1);
        stream.read(&buffer[0], static_cast<std::streamsize>(buffer.size()));

        boost::algorithm::trim(buffer);

        auto itr = buffer.begin();
        auto end = buffer.end();

        bool r = false;
        try
        {
            r = boost::spirit::qi::parse(
                itr, end, +boost::spirit::qi::int_ % +boost::spirit::qi::space, _data);
        }
        catch (std::exception const &ex)
        {
            throw osrm::exception(
                std::string("Failed to read from raster source with exception: ") + ex.what());
        }

        if (!r || itr != end)
        {
            throw osrm::exception("Failed to parse raster source correctly.");
        }
    }

    RasterGrid(const RasterGrid &) = default;
    RasterGrid &operator=(const RasterGrid &) = default;

    RasterGrid(RasterGrid &&) = default;
    RasterGrid &operator=(RasterGrid &&) = default;

    std::int32_t operator()(std::size_t x, std::size_t y) { return _data[y * xdim + x]; }
    std::int32_t operator()(std::size_t x, std::size_t y) const { return _data[(y)*xdim + (x)]; }

  private:
    std::vector<std::int32_t> _data;
    std::size_t xdim, ydim;
};

/**
    \brief Stores raster source data in memory and provides lookup functions.
*/
class RasterSource
{
  private:
    const float xstep;
    const float ystep;

    float calcSize(int min, int max, std::size_t count) const;

  public:
    RasterGrid raster_data;

    const std::size_t width;
    const std::size_t height;
    const int xmin;
    const int xmax;
    const int ymin;
    const int ymax;

    RasterDatum getRasterData(const int lon, const int lat) const;

    RasterDatum getRasterInterpolate(const int lon, const int lat) const;

    RasterSource(RasterGrid _raster_data,
                 std::size_t width,
                 std::size_t height,
                 int _xmin,
                 int _xmax,
                 int _ymin,
                 int _ymax);
};

class SourceContainer
{
  public:
    SourceContainer() = default;

    int loadRasterSource(const std::string &path_string,
                         double xmin,
                         double xmax,
                         double ymin,
                         double ymax,
                         std::size_t nrows,
                         std::size_t ncols);

    RasterDatum getRasterDataFromSource(unsigned int source_id, int lon, int lat);

    RasterDatum getRasterInterpolateFromSource(unsigned int source_id, int lon, int lat);

  private:
    std::vector<RasterSource> LoadedSources;
    std::unordered_map<std::string, int> LoadedSourcePaths;
};

#endif /* RASTER_SOURCE_HPP */

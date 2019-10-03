#ifndef RASTER_SOURCE_HPP
#define RASTER_SOURCE_HPP

#include "util/coordinate.hpp"
#include "util/exception.hpp"

#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/assert.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_int.hpp>
#include <boost/foreach.hpp>

#include <storage/io.hpp>

#include <iterator>
#include <unordered_map>
#include <string>
#include <list>
#include <iostream>
using namespace std;

namespace osrm
{
namespace extractor
{

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
        BOOST_ASSERT(ydim * xdim <= _data.capacity());

        // Construct FileReader
        storage::io::FileReader file_reader(filepath, storage::io::FileReader::HasNoFingerprint);
        std::string buf;
        buf.resize(xdim * 11); // INT32_MAX = 2147483647 = 10 chars + 1 white space = 11
        BOOST_ASSERT(xdim * 11 <= buf.size());

        for (unsigned int y = 0 ; y < ydim ; y++) {
            // read one line from file.
            file_reader.ReadLine(&buf[0], xdim * 11);
            boost::algorithm::trim(buf);

            std::vector<std::string> result;
            std::string delim (" ");
            //boost::split(result, buf, boost::is_any_of(delim), boost::algorithm::token_compress_on);
            boost::split(result, buf, boost::is_any_of(delim));
            unsigned int x = 0;
            BOOST_FOREACH(std::string s, result) {
                _data[(y * xdim) + x] = atoi(s.c_str());
                ++x;
            }
            BOOST_ASSERT(x == xdim);
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

    float CalcSize(int min, int max, std::size_t count) const;

  public:
    RasterGrid raster_data;

    const std::size_t width;
    const std::size_t height;
    const int xmin;
    const int xmax;
    const int ymin;
    const int ymax;

    RasterDatum GetRasterData(const int lon, const int lat) const;

    RasterDatum GetRasterInterpolate(const int lon, const int lat) const;

    RasterSource(RasterGrid _raster_data,
                 std::size_t width,
                 std::size_t height,
                 int _xmin,
                 int _xmax,
                 int _ymin,
                 int _ymax);
};

class RasterContainer
{
  public:
    RasterContainer() = default;

    int LoadRasterSource(const std::string &path_string,
                         double xmin,
                         double xmax,
                         double ymin,
                         double ymax,
                         std::size_t nrows,
                         std::size_t ncols);

    RasterDatum GetRasterDataFromSource(unsigned int source_id, double lon, double lat);

    RasterDatum GetRasterInterpolateFromSource(unsigned int source_id, double lon, double lat);

  private:
    static std::vector<RasterSource> LoadedSources;
    static std::unordered_map<std::string, int> LoadedSourcePaths;
};
}
}

#endif /* RASTER_SOURCE_HPP */

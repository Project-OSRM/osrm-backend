#include "util/timezones.hpp"
#include "util/exception.hpp"
#include "util/log.hpp"

#include <boost/scope_exit.hpp>

#ifdef ENABLE_SHAPEFILE
#include <shapefil.h>
#endif

#include <string>
#include <unordered_map>

// Function loads time zone shape polygons, computes a zone local time for utc_time,
// creates a lookup R-tree and returns a lambda function that maps a point
// to the corresponding local time
namespace osrm
{
namespace updater
{
bool SupportsShapefiles()
{
#ifdef ENABLE_SHAPEFILE
    return true;
#else
    return false;
#endif
}

std::function<struct tm(const point_t &)> LoadLocalTimesRTree(const std::string &tz_shapes_filename,
                                                              std::time_t utc_time)
{
#ifdef ENABLE_SHAPEFILE
    // Load time zones shapes and collect local times of utc_time
    auto shphandle = SHPOpen(tz_shapes_filename.c_str(), "rb");
    auto dbfhandle = DBFOpen(tz_shapes_filename.c_str(), "rb");

    BOOST_SCOPE_EXIT(&shphandle, &dbfhandle)
    {
        DBFClose(dbfhandle);
        SHPClose(shphandle);
    }
    BOOST_SCOPE_EXIT_END

    if (!shphandle || !dbfhandle)
    {
        throw osrm::util::exception("failed to open " + tz_shapes_filename + ".shp or " +
                                    tz_shapes_filename + ".dbf file");
    }

    int num_entities, shape_type;
    SHPGetInfo(shphandle, &num_entities, &shape_type, NULL, NULL);
    if (num_entities != DBFGetRecordCount(dbfhandle))
    {
        throw osrm::util::exception("inconsistent " + tz_shapes_filename + ".shp and " +
                                    tz_shapes_filename + ".dbf files");
    }

    const auto tzid = DBFGetFieldIndex(dbfhandle, "TZID");
    if (tzid == -1)
    {
        throw osrm::util::exception("did not find field called 'TZID' in the " +
                                    tz_shapes_filename + ".dbf file");
    }

    // Lambda function that returns local time in the tzname time zone
    // Thread safety: MT-Unsafe const:env
    std::unordered_map<std::string, struct tm> local_time_memo;
    auto get_local_time_in_tz = [utc_time, &local_time_memo](const char *tzname) {
        auto it = local_time_memo.find(tzname);
        if (it == local_time_memo.end())
        {
            struct tm timeinfo;
            setenv("TZ", tzname, 1);
            tzset();
            localtime_r(&utc_time, &timeinfo);
            it = local_time_memo.insert({tzname, timeinfo}).first;
        }

        return it->second;
    };

    // Get all time zone shapes and save local times in a vector
    std::vector<rtree_t::value_type> polygons;
    std::vector<local_time_t> local_times;
    for (int shape = 0; shape < num_entities; ++shape)
    {
        auto object = SHPReadObject(shphandle, shape);
        BOOST_SCOPE_EXIT(&object) { SHPDestroyObject(object); }
        BOOST_SCOPE_EXIT_END

        if (object && object->nSHPType == SHPT_POLYGON)
        {
            // Find time zone polygon and place its bbox in into R-Tree
            polygon_t polygon;
            for (int vertex = 0; vertex < object->nVertices; ++vertex)
            {
                polygon.outer().emplace_back(object->padfX[vertex], object->padfY[vertex]);
            }

            polygons.emplace_back(boost::geometry::return_envelope<box_t>(polygon),
                                  local_times.size());

            // Get time zone name and emplace polygon and local time for the UTC input
            const auto tzname = DBFReadStringAttribute(dbfhandle, shape, tzid);
            local_times.emplace_back(local_time_t{polygon, get_local_time_in_tz(tzname)});

            // std::cout << boost::geometry::dsv(boost::geometry::return_envelope<box_t>(polygon))
            //           << " " << tzname << " " << asctime(&local_times.back().second);
        }
    }

    // Create R-tree for collected shape polygons
    rtree_t rtree(polygons);

    // Return a lambda function that maps the input point and UTC time to the local time
    // binds rtree and local_times
    return [rtree, local_times](const point_t &point) {
        std::vector<rtree_t::value_type> result;
        rtree.query(boost::geometry::index::intersects(point), std::back_inserter(result));
        for (const auto v : result)
        {
            const auto index = v.second;
            if (boost::geometry::within(point, local_times[index].first))
                return local_times[index].second;
        }
        return tm{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    };
#endif
}

Timezoner::Timezoner(std::string tz_filename, std::time_t utc_time_now)
{
    util::Log() << "Time zone validation based on UTC time : " << utc_time_now;
    GetLocalTime = LoadLocalTimesRTree(tz_filename, utc_time_now);
}

Timezoner::Timezoner(std::string tz_filename)
    : Timezoner(tz_filename, std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()))
{
}
}
}

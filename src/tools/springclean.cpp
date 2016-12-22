#include <cstdio>

#include "storage/shared_datatype.hpp"
#include "storage/shared_memory.hpp"
#include "util/log.hpp"

namespace osrm
{
namespace tools
{

// FIXME remove after folding back into datastore
using namespace storage;

void deleteRegion(const SharedDataType region)
{
    if (SharedMemory::RegionExists(region) && !SharedMemory::Remove(region))
    {
        const std::string name = [&] {
            switch (region)
            {
            case CURRENT_REGION:
                return "CURRENT_REGIONS";
            case REGION_1:
                return "REGION_1";
            case REGION_2:
                return "REGION_2";
            default: // REGION_NONE:
                return "REGION_NONE";
            }
        }();

        util::Log(logWARNING) << "could not delete shared memory region " << name;
    }
}

// find all existing shmem regions and remove them.
void springclean()
{
    util::Log() << "spring-cleaning all shared memory regions";
    deleteRegion(REGION_1);
    deleteRegion(REGION_2);
    deleteRegion(CURRENT_REGION);
}
}
}

int main()
{
    osrm::util::LogPolicy::GetInstance().Unmute();
    osrm::util::Log() << "Releasing all locks";
    osrm::util::Log() << "ATTENTION! BE CAREFUL!";
    osrm::util::Log() << "----------------------";
    osrm::util::Log() << "This tool may put osrm-routed into an undefined state!";
    osrm::util::Log() << "Type 'Y' to acknowledge that you know what your are doing.";
    osrm::util::Log() << "\n\nDo you want to purge all shared memory allocated "
                      << "by osrm-datastore? [type 'Y' to confirm]";

    const auto letter = getchar();
    if (letter != 'Y')
    {
        osrm::util::Log() << "aborted.";
        return EXIT_SUCCESS;
    }
    osrm::tools::springclean();
    return EXIT_SUCCESS;
}

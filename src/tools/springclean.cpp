#include <cstdio>

#include "datastore/shared_memory_factory.hpp"
#include "engine/datafacade/shared_datatype.hpp"
#include "util/simple_logger.hpp"

namespace osrm
{
namespace tools
{

// FIXME remove after folding back into datastore
using namespace datastore;
using namespace engine::datafacade;

void deleteRegion(const SharedDataType region)
{
    if (SharedMemory::RegionExists(region) && !SharedMemory::Remove(region))
    {
        const std::string name = [&]
        {
            switch (region)
            {
            case CURRENT_REGIONS:
                return "CURRENT_REGIONS";
            case LAYOUT_1:
                return "LAYOUT_1";
            case DATA_1:
                return "DATA_1";
            case LAYOUT_2:
                return "LAYOUT_2";
            case DATA_2:
                return "DATA_2";
            case LAYOUT_NONE:
                return "LAYOUT_NONE";
            default: // DATA_NONE:
                return "DATA_NONE";
            }
        }();

        util::SimpleLogger().Write(logWARNING) << "could not delete shared memory region " << name;
    }
}

// find all existing shmem regions and remove them.
void springclean()
{
    util::SimpleLogger().Write() << "spring-cleaning all shared memory regions";
    deleteRegion(DATA_1);
    deleteRegion(LAYOUT_1);
    deleteRegion(DATA_2);
    deleteRegion(LAYOUT_2);
    deleteRegion(CURRENT_REGIONS);
}
}
}

int main()
{
    osrm::util::LogPolicy::GetInstance().Unmute();
    try
    {
        osrm::util::SimpleLogger().Write() << "Releasing all locks";
        osrm::util::SimpleLogger().Write() << "ATTENTION! BE CAREFUL!";
        osrm::util::SimpleLogger().Write() << "----------------------";
        osrm::util::SimpleLogger().Write()
            << "This tool may put osrm-routed into an undefined state!";
        osrm::util::SimpleLogger().Write()
            << "Type 'Y' to acknowledge that you know what your are doing.";
        osrm::util::SimpleLogger().Write()
            << "\n\nDo you want to purge all shared memory allocated "
            << "by osrm-datastore? [type 'Y' to confirm]";

        const auto letter = getchar();
        if (letter != 'Y')
        {
            osrm::util::SimpleLogger().Write() << "aborted.";
            return 0;
        }
        osrm::tools::springclean();
    }
    catch (const std::exception &e)
    {
        osrm::util::SimpleLogger().Write(logWARNING) << "[excpetion] " << e.what();
    }
    return 0;
}

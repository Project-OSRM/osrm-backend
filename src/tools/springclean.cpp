#include <cstdio>

#include "datastore/shared_memory_factory.hpp"
#include "engine/datafacade/shared_datatype.hpp"
#include "util/version.hpp"
#include "util/simple_logger.hpp"

void delete_region(const SharedDataType region)
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

        SimpleLogger().Write(logWARNING) << "could not delete shared memory region " << name;
    }
}

// find all existing shmem regions and remove them.
void springclean()
{
    SimpleLogger().Write() << "spring-cleaning all shared memory regions";
    delete_region(DATA_1);
    delete_region(LAYOUT_1);
    delete_region(DATA_2);
    delete_region(LAYOUT_2);
    delete_region(CURRENT_REGIONS);
}

int main()
{
    LogPolicy::GetInstance().Unmute();
    try
    {
        SimpleLogger().Write() << "starting up engines, " << OSRM_VERSION << "\n\n";
        SimpleLogger().Write() << "Releasing all locks";
        SimpleLogger().Write() << "ATTENTION! BE CAREFUL!";
        SimpleLogger().Write() << "----------------------";
        SimpleLogger().Write() << "This tool may put osrm-routed into an undefined state!";
        SimpleLogger().Write() << "Type 'Y' to acknowledge that you know what your are doing.";
        SimpleLogger().Write() << "\n\nDo you want to purge all shared memory allocated "
                               << "by osrm-datastore? [type 'Y' to confirm]";

        const auto letter = getchar();
        if (letter != 'Y')
        {
            SimpleLogger().Write() << "aborted.";
            return 0;
        }
        springclean();
    }
    catch (const std::exception &e)
    {
        SimpleLogger().Write(logWARNING) << "[excpetion] " << e.what();
    }
    return 0;
}

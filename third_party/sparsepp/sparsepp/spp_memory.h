#if !defined(spp_memory_h_guard)
#define spp_memory_h_guard

#include <cstdint>
#include <cstring>
#include <cstdlib>

#if defined(_WIN32) || defined( __CYGWIN__)
    #define SPP_WIN
#endif

#ifdef SPP_WIN
    #include <windows.h>
    #include <Psapi.h>
    #undef min
    #undef max
#else
    #include <sys/types.h>
    #include <sys/sysinfo.h>
#endif

namespace spp
{
    uint64_t GetSystemMemory()
    {
#ifdef SPP_WIN
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&memInfo);
        return static_cast<uint64_t>(memInfo.ullTotalPageFile);
#else
        struct sysinfo memInfo;
        sysinfo (&memInfo);
        auto totalVirtualMem = memInfo.totalram;

        totalVirtualMem += memInfo.totalswap;
        totalVirtualMem *= memInfo.mem_unit;
        return static_cast<uint64_t>(totalVirtualMem);
#endif
    }

    uint64_t GetTotalMemoryUsed()
    {
#ifdef SPP_WIN
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&memInfo);
        return static_cast<uint64_t>(memInfo.ullTotalPageFile - memInfo.ullAvailPageFile);
#else
        struct sysinfo memInfo;
        sysinfo(&memInfo);
        auto virtualMemUsed = memInfo.totalram - memInfo.freeram;

        virtualMemUsed += memInfo.totalswap - memInfo.freeswap;
        virtualMemUsed *= memInfo.mem_unit;

        return static_cast<uint64_t>(virtualMemUsed);
#endif
    }

    uint64_t GetProcessMemoryUsed()
    {
#ifdef SPP_WIN
        PROCESS_MEMORY_COUNTERS_EX pmc;
        GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PPROCESS_MEMORY_COUNTERS>(&pmc), sizeof(pmc));
        return static_cast<uint64_t>(pmc.PrivateUsage);
#else
        auto parseLine = 
            [](char* line)->int
            {
                auto i = strlen(line);
				
                while(*line < '0' || *line > '9') 
                {
                    line++;
                }

                line[i-3] = '\0';
                i = atoi(line);
                return i;
            };

        auto file = fopen("/proc/self/status", "r");
        auto result = -1;
        char line[128];

        while(fgets(line, 128, file) != nullptr)
        {
            if(strncmp(line, "VmSize:", 7) == 0)
            {
                result = parseLine(line);
                break;
            }
        }

        fclose(file);
        return static_cast<uint64_t>(result) * 1024;
#endif
    }

    uint64_t GetPhysicalMemory()
    {
#ifdef SPP_WIN
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&memInfo);
        return static_cast<uint64_t>(memInfo.ullTotalPhys);
#else
        struct sysinfo memInfo;
        sysinfo(&memInfo);

        auto totalPhysMem = memInfo.totalram;

        totalPhysMem *= memInfo.mem_unit;
        return static_cast<uint64_t>(totalPhysMem);
#endif
    }

}

#endif // spp_memory_h_guard

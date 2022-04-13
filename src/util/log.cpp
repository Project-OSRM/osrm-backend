#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>

#include "util/log.hpp"
#include "util/isatty.hpp"
#include <boost/algorithm/string/predicate.hpp>
#include <cstdio>
#include <ctime>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <mutex>
#include <string>
#include <iomanip>
#include "storage/shared_memory.hpp"
#include <boost/interprocess/xsi_shared_memory.hpp>

namespace osrm
{
namespace util
{

namespace
{
static const char COL_RESET[]{"\x1b[0m"};
static const char RED[]{"\x1b[31m"};
static const char YELLOW[]{"\x1b[33m"};
#ifdef ENABLE_DEBUG_LOGGING
static const char MAGENTA[]{"\x1b[35m"};
#endif
// static const char GREEN[] { "\x1b[32m"};
// static const char BLUE[] { "\x1b[34m"};
// static const char CYAN[] { "\x1b[36m"};
} // namespace

void LogPolicy::Unmute() { m_is_mute = false; }

void LogPolicy::Mute() { m_is_mute = true; }

bool LogPolicy::IsMute() const { return m_is_mute; }

LogLevel LogPolicy::GetLevel() const { return m_level; }

void LogPolicy::SetLevel(LogLevel level) { m_level = level; }

void LogPolicy::SetLevel(std::string const &level)
{
    // Keep in sync with LogLevel definition
    if (boost::iequals(level, "NONE"))
        m_level = logNONE;
    else if (boost::iequals(level, "ERROR"))
        m_level = logERROR;
    else if (boost::iequals(level, "WARNING"))
        m_level = logWARNING;
    else if (boost::iequals(level, "INFO"))
        m_level = logINFO;
    else if (boost::iequals(level, "DEBUG"))
        m_level = logDEBUG;
    else
        m_level = logINFO;
}

LogPolicy &LogPolicy::GetInstance()
{
    static LogPolicy runningInstance;
    return runningInstance;
}

std::string LogPolicy::GetLevels()
{
    // Keep in sync with LogLevel definition
    return "NONE, ERROR, WARNING, INFO, DEBUG";
}

Log::Log(LogLevel level_, std::ostream &ostream) : level(level_), stream(ostream)
{
    std::lock_guard<std::mutex> lock(get_mutex());
    if (!LogPolicy::GetInstance().IsMute() && level <= LogPolicy::GetInstance().GetLevel())
    {
        std::chrono::system_clock::time_point now_ts = std::chrono::system_clock::now();
        std::time_t now = std::chrono::system_clock::to_time_t(now_ts);
        const long ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now_ts).time_since_epoch().count() % 1000;
        stream << "[" << std::put_time(std::localtime(&now), "%H:%M:%S.") << ms << "] [pid: " << syscall(SYS_gettid) << "] [tid: " << pthread_self() << "] ";
        const bool is_terminal = IsStdoutATTY();
        switch (level)
        {
        case logNONE:
            break;
        case logWARNING:
            stream << (is_terminal ? YELLOW : "") << "[warn] ";
            break;
        case logERROR:
            stream << (is_terminal ? RED : "") << "[error] ";
            break;
        case logDEBUG:
#ifdef ENABLE_DEBUG_LOGGING
            stream << (is_terminal ? MAGENTA : "") << "[debug] ";
#endif
            break;
        default: // logINFO:
            stream << "[info] ";
            break;
        }
    }
}

Log::Log(LogLevel level_) : Log(level_, buffer) {}

std::mutex &Log::get_mutex()
{
    static std::mutex mtx;
    return mtx;
}

/**
 * Close down this logging instance.
 * This destructor is responsible for flushing any buffered data,
 * and printing a newline character (each logger object is responsible for only one line)
 * Because sub-classes can replace the `stream` object - we need to verify whether
 * we're writing to std::cerr/cout, or whether we should write to the stream
 */
Log::~Log()
{
    std::lock_guard<std::mutex> lock(get_mutex());
    if (!LogPolicy::GetInstance().IsMute() && level <= LogPolicy::GetInstance().GetLevel())
    {
        const bool usestd = (&stream == &buffer);
        const bool is_terminal = IsStdoutATTY();
        if (usestd)
        {
            switch (level)
            {
            case logNONE:
                break;
            case logWARNING:
            case logERROR:
                std::cerr << buffer.str();
                std::cerr << (is_terminal ? COL_RESET : "");
                std::cerr << std::endl;
                break;
            case logDEBUG:
#ifndef ENABLE_DEBUG_LOGGING
                break;
#endif
            case logINFO:
            default:
                std::cout << buffer.str();
                std::cout << (is_terminal ? COL_RESET : "");
                std::cout << std::endl;
                break;
            }
        }
        else
        {
            stream << (is_terminal ? COL_RESET : "");
            stream << std::endl;
        }
    }
    stream.flush();
}

UnbufferedLog::UnbufferedLog(LogLevel level_)
    : Log(level_, (level_ == logWARNING || level_ == logERROR) ? std::cerr : std::cout)
{
    stream.flags(std::ios_base::unitbuf);
}

std::string toHexString(int a) {
    std::ostringstream oss;
    oss << "0x" << std::setw(8) << std::setfill('0') << std::hex << a;
    return oss.str();
}

std::string shmKeyToString(std::uint16_t shm_key) {
    std::ostringstream oss;
    osrm::storage::OSRMLockFile lock_file;
    oss << shm_key << " (";
    try {
        oss << "file: " << lock_file(shm_key);
        boost::interprocess::xsi_key key(lock_file(shm_key).string().c_str(), shm_key);
        oss << ", key: " << toHexString(key.get_key());
        bool exists = storage::SharedMemory::RegionExists(shm_key);
        oss << ", exists: " << exists;
    } catch (const std::exception& e) {
        oss << "error: " << e.what();
        std::string path = lock_file(shm_key).string();
        oss << ", path: " << path << ", exists: " << boost::filesystem::exists(path);
        key_t k = ::ftok(path.c_str(), shm_key);
        oss << ", ftok: " << k << ", errno: " << errno;
    }
    oss << ")";
    return oss.str();
}

} // namespace util
} // namespace osrm

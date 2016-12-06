#include "util/log.hpp"
#include "util/isatty.hpp"
#include <cstdio>
#include <iostream>
#include <mutex>
#include <string>

namespace osrm
{
namespace util
{

namespace
{
static const char COL_RESET[]{"\x1b[0m"};
static const char RED[]{"\x1b[31m"};
static const char YELLOW[]{"\x1b[33m"};
#ifndef NDEBUG
static const char MAGENTA[]{"\x1b[35m"};
#endif
// static const char GREEN[] { "\x1b[32m"};
// static const char BLUE[] { "\x1b[34m"};
// static const char CYAN[] { "\x1b[36m"};
}

void LogPolicy::Unmute() { m_is_mute = false; }

void LogPolicy::Mute() { m_is_mute = true; }

bool LogPolicy::IsMute() const { return m_is_mute; }

LogPolicy &LogPolicy::GetInstance()
{
    static LogPolicy runningInstance;
    return runningInstance;
}

Log::Log(LogLevel level_, std::ostream &ostream) : level(level_), stream(ostream)
{
    const bool is_terminal = IsStdoutATTY();
    std::lock_guard<std::mutex> lock(get_mutex());
    switch (level)
    {
    case logWARNING:
        stream << (is_terminal ? YELLOW : "") << "[warn] ";
        break;
    case logERROR:
        stream << (is_terminal ? RED : "") << "[error] ";
        break;
    case logDEBUG:
#ifndef NDEBUG
        stream << (is_terminal ? MAGENTA : "") << "[debug] ";
#endif
        break;
    default: // logINFO:
        stream << "[info] ";
        break;
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
    const bool usestd = (&stream == &buffer);
    if (!LogPolicy::GetInstance().IsMute())
    {
        const bool is_terminal = IsStdoutATTY();
        if (usestd)
        {
            switch (level)
            {
            case logWARNING:
            case logERROR:
                std::cerr << buffer.str();
                std::cerr << (is_terminal ? COL_RESET : "");
                std::cerr << std::endl;
                break;
            case logDEBUG:
#ifdef NDEBUG
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
}

UnbufferedLog::UnbufferedLog(LogLevel level_)
    : Log(level_, (level_ == logWARNING || level_ == logERROR) ? std::cerr : std::cout)
{
    stream.flags(std::ios_base::unitbuf);
}
}
}

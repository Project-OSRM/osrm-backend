#ifndef LOG_HPP
#define LOG_HPP

#include <atomic>
#include <mutex>
#include <sstream>

enum LogLevel
{
    logINFO,
    logWARNING,
    logERROR,
    logDEBUG
};

namespace osrm
{
namespace util
{

class LogPolicy
{
  public:
    void Unmute();

    void Mute();

    bool IsMute() const;

    static LogPolicy &GetInstance();

    LogPolicy(const LogPolicy &) = delete;
    LogPolicy &operator=(const LogPolicy &) = delete;

  private:
    LogPolicy() : m_is_mute(true) {}
    std::atomic<bool> m_is_mute;
};

class Log
{
  public:
    Log(LogLevel level_ = logINFO);
    Log(LogLevel level_, std::ostream &ostream);

    virtual ~Log();
    std::mutex &get_mutex();

    template <typename T> inline std::ostream &operator<<(const T &data) { return stream << data; }

  protected:
    LogLevel level;
    std::ostringstream buffer;
    std::ostream &stream;
};

/**
 * Modified logger - this one doesn't buffer - it writes directly to stdout,
 * and the final newline is only printed when the object is destructed.
 * Useful for logging situations where you don't want to newline right away
 */
class UnbufferedLog : public Log
{
  public:
    UnbufferedLog(LogLevel level_ = logINFO);
};
}
}

#endif /* LOG_HPP */

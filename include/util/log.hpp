#ifndef LOG_HPP
#define LOG_HPP

#include <atomic>
#include <mutex>
#include <sstream>

enum LogLevel
{
    logNONE,
    logERROR,
    logWARNING,
    logINFO,
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

    LogLevel GetLevel() const;
    void SetLevel(LogLevel level);
    void SetLevel(std::string const &level);

    static LogPolicy &GetInstance();
    static std::string GetLevels();

    LogPolicy(const LogPolicy &) = delete;
    LogPolicy &operator=(const LogPolicy &) = delete;

  private:
    LogPolicy() : m_is_mute(true), m_level(logINFO) {}
    std::atomic<bool> m_is_mute;
    LogLevel m_level;
};

class Log
{
  public:
    Log(LogLevel level_ = logINFO);
    Log(LogLevel level_, std::ostream &ostream);

    virtual ~Log();
    std::mutex &get_mutex();

    template <typename T> inline Log &operator<<(const T &data)
    {
        const auto &policy = LogPolicy::GetInstance();
        if (!policy.IsMute() && level <= policy.GetLevel())
        {
            stream << data;
        }
        return *this;
    }

    template <typename T> inline Log &operator<<(const std::atomic<T> &data)
    {
        const auto &policy = LogPolicy::GetInstance();
        if (!policy.IsMute() && level <= policy.GetLevel())
        {
            stream << T(data);
        }
        return *this;
    }

    typedef std::ostream &(manip)(std::ostream &);

    inline Log &operator<<(manip &m)
    {
        const auto &policy = LogPolicy::GetInstance();
        if (!policy.IsMute() && level <= policy.GetLevel())
        {
            stream << m;
        }
        return *this;
    }

  protected:
    const LogLevel level;
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
} // namespace util
} // namespace osrm

#endif /* LOG_HPP */

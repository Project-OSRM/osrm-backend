#ifndef SIMPLE_LOGGER_HPP
#define SIMPLE_LOGGER_HPP

#include <atomic>
#include <mutex>
#include <sstream>

enum LogLevel
{
    logINFO,
    logWARNING,
    logDEBUG
};

class LogPolicy
{
  public:
    void Unmute();

    void Mute();

    bool IsMute() const;

    static LogPolicy &GetInstance();

    LogPolicy(const LogPolicy &) = delete;

  private:
    LogPolicy() : m_is_mute(true) {}
    std::atomic<bool> m_is_mute;
};

class SimpleLogger
{
  public:
    SimpleLogger();

    virtual ~SimpleLogger();
    std::mutex &get_mutex();
    std::ostringstream &Write(LogLevel l = logINFO) noexcept;

  private:
    std::ostringstream os;
    LogLevel level;
};

#endif /* SIMPLE_LOGGER_HPP */

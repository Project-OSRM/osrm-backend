#ifndef PERCENT_HPP
#define PERCENT_HPP

#include <atomic>
#include <iostream>

#include "util/isatty.hpp"
#include "util/log.hpp"

namespace osrm
{
namespace util
{

class Percent
{
    Log &log;

  public:
    explicit Percent(Log &log_, unsigned max_value, unsigned step = 5) : log{log_}
    {
        Reinit(max_value, step);
    }

    // Reinitializes
    void Reinit(unsigned max_value, unsigned step = 5)
    {
        m_max_value = max_value;
        m_current_value = 0;
        m_percent_interval = m_max_value / 100;
        m_next_threshold = m_percent_interval;
        m_last_percent = 0;
        m_step = step;
    }

    // If there has been significant progress, display it.
    void PrintStatus(unsigned current_value)
    {
        if (current_value >= m_next_threshold)
        {
            m_next_threshold += m_percent_interval;
            PrintPercent(current_value / static_cast<double>(m_max_value) * 100.);
        }
        if (current_value + 1 == m_max_value)
            log << " 100%";
    }

    void PrintIncrement()
    {
        ++m_current_value;
        PrintStatus(m_current_value);
    }

    void PrintAddition(const unsigned addition)
    {
        m_current_value += addition;
        PrintStatus(m_current_value);
    }

  private:
    std::atomic_uint m_current_value;
    unsigned m_max_value;
    unsigned m_percent_interval;
    unsigned m_next_threshold;
    unsigned m_last_percent;
    unsigned m_step;

    // Displays progress.
    void PrintPercent(double percent)
    {
        while (percent >= m_last_percent + m_step)
        {
            m_last_percent += m_step;
            if (m_last_percent % 10 == 0)
            {
                log << " " << m_last_percent << "% ";
            }
            else
            {
                log << ".";
            }

            // When not on a TTY, print newlines after each progress indicator so
            // so that progress is visible to line-buffered logging systems
            if (!IsStdoutATTY())
                log << std::endl;
        }
    }
};
} // namespace util
} // namespace osrm

#endif // PERCENT_HPP

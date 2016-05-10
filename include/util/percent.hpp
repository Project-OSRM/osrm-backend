#ifndef PERCENT_HPP
#define PERCENT_HPP

#include <iostream>
#include <atomic>

namespace osrm
{
namespace util
{

class Percent
{
  public:
    explicit Percent(unsigned max_value, unsigned step = 5) { Reinit(max_value, step); }

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
            std::cout << " 100%" << std::endl;
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
                std::cout << " " << m_last_percent << "% ";
            }
            else
            {
                std::cout << ".";
            }
            std::cout.flush();
        }
    }
};
}
}

#endif // PERCENT_HPP

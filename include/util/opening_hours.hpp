#ifndef OSRM_OPENING_HOURS_HPP
#define OSRM_OPENING_HOURS_HPP

#include <boost/date_time/gregorian/gregorian.hpp>

#include <string>
#include <vector>

namespace osrm
{
namespace util
{

// Helper classes for "opening hours" format http://wiki.openstreetmap.org/wiki/Key:opening_hours
// Grammar https://wiki.openstreetmap.org/wiki/Key:opening_hours/specification
// Supported simplified features in CheckOpeningHours:
// - Year/Month/Day ranges
// - Weekday ranges
// - Time ranges
// Not supported:
// - Week numbers
// - Holidays, events, variables dates
// - Day offsets and periodic ranges
struct OpeningHours
{
    enum Modifier
    {
        unknown,
        open,
        closed,
        off,
        is24_7
    };

    struct Time
    {
        enum Event : unsigned char
        {
            invalid,
            none,
            dawn,
            sunrise,
            sunset,
            dusk
        };

        Event event;
        std::int32_t minutes;

        Time() : event(invalid), minutes(0) {}
        Time(Event event) : event(event), minutes(0) {}
        Time(char hour, char min) : event(none), minutes(hour * 60 + min) {}
        Time(Event event, bool positive, const Time &offset)
            : event(event), minutes(positive ? offset.minutes : -offset.minutes)
        {
        }
    };

    struct TimeSpan
    {
        Time from, to;
        TimeSpan() = default;
        TimeSpan(const Time &from_, const Time &to_) : from(from_), to(to_)
        {
            if (to.minutes < from.minutes)
                to.minutes += 24 * 60;
        }

        bool IsInRange(const struct tm &time, bool &use_curr_day, bool &use_next_day) const
        {
            // TODO: events are not handled
            if (from.event != OpeningHours::Time::none || to.event != OpeningHours::Time::none)
                return false;

            const auto minutes = time.tm_hour * 60 + time.tm_min;
            if (to.minutes > 24 * 60)
            {
                use_curr_day = (from.minutes <= minutes); // in range [from, 24:00) current day
                use_next_day = (minutes < to.minutes - 24 * 60); // in range [00:00, to) next day
            }
            else
            {
                use_curr_day =
                    (from.minutes <= minutes && minutes < to.minutes); // in range [from, to)
                use_next_day = false;                                  // do not use the next day
            }

            return use_curr_day || use_next_day;
        }
    };

    struct WeekdayRange
    {
        int weekdays, overnight_weekdays;
        WeekdayRange() = default;
        WeekdayRange(unsigned char from, unsigned char to)
        {
            // weekdays mask for [from, to], e.g [2, 5] -> 0111100, [5, 2] -> 1100111,
            //  [3, 3] -> 0001000, [0,6] -> 1111111, [6,0] -> 1000001, [4, 3] -> 1111111
            weekdays = (from <= to) ? ((1 << (to - from + 1)) - 1) << from
                                    : ~(((1 << (from - to - 1)) - 1) << (to + 1));
            weekdays &= 0x7f;
            overnight_weekdays = (weekdays << 1) | (weekdays & 0x40 ? 1 : 0);
        }

        bool IsInRange(const struct tm &time, bool use_curr_day, bool use_next_day) const
        {
            return (use_curr_day && weekdays & (1 << time.tm_wday)) ||
                   (use_next_day && overnight_weekdays & (1 << time.tm_wday));
        }
    };

    struct Monthday
    {
        int year;
        char month;
        char day;
        Monthday() = default;
        Monthday(int year) : year(year), month(0), day(0) {}
        Monthday(int year, char month, char day) : year(year), month(month), day(day) {}

        bool IsValid() const { return year > 0 || month != 0 || day != 0; }
        bool operator==(const Monthday &rhs) const
        {
            return std::tie(year, month, day) == std::tie(rhs.year, rhs.month, rhs.day);
        }
    };

    struct MonthdayRange
    {
        Monthday from, to;
        MonthdayRange() : from(0, 0, 0), to(0, 0, 0) {}
        MonthdayRange(const Monthday &from, const Monthday &to) : from(from), to(to) {}

        bool IsInRange(const struct tm &time, bool use_curr_day, bool use_next_day) const
        {
            using boost::gregorian::date;
            using boost::gregorian::date_duration;

            const auto year = time.tm_year + 1900;
            const auto month = time.tm_mon + 1;

            date date_current(year, month, time.tm_mday);
            date date_from(boost::gregorian::min_date_time);
            date date_to(boost::gregorian::max_date_time);

            if (from.IsValid())
            {
                date_from = (from.day == 0) ? date(from.year == 0 ? year : from.year,
                                                   from.month == 0 ? month : from.month,
                                                   1)
                                            : date(from.year == 0 ? year : from.year,
                                                   from.month == 0 ? month : from.month,
                                                   from.day);
            }
            if (to.IsValid())
            {
                date_to = date(to.year == 0 ? (from.year == 0 ? year : from.year) : to.year,
                               to.month == 0 ? (from.month == 0 ? month : from.month) : to.month,
                               1);
                date_to = (to.day == 0) ? date_to.end_of_month()
                                        : date(date_to.year(), date_to.month(), to.day);
            }
            else if (to == Monthday())
            {
                date_to = date_from;
            }

            const bool inverse = (from.year == 0) && (to.year == 0) && (date_from > date_to);
            if (inverse)
            {
                std::swap(date_from, date_to);
            }

            if (!use_curr_day)
                date_from += date_duration(1);
            if (use_next_day && date_to != date(boost::gregorian::max_date_time))
                date_to += date_duration(1);

            return (date_from <= date_current && date_current <= date_to) ^ inverse;
        }
    };

    OpeningHours() : modifier(open) {}

    bool IsInRange(const struct tm &time) const
    {
        bool use_curr_day = true;  // the first matching time uses the current day
        bool use_next_day = false; // the first matching time uses the next day
        return (!times.empty() || !weekdays.empty() || !monthdays.empty())
               // the value is in range if time is not specified or is in any time range
               // (also modifies use_curr_day and use_next_day flags to handle overnight day ranges,
               // e.g. for 22:00-03:00 and 2am -> use_curr_day = false and use_next_day = true)
               && (times.empty() ||
                   std::any_of(times.begin(),
                               times.end(),
                               [&time, &use_curr_day, &use_next_day](const auto &x) {
                                   return x.IsInRange(time, use_curr_day, use_next_day);
                               }))
               // .. and if weekdays are not specified or matches weekdays range
               && (weekdays.empty() ||
                   std::any_of(weekdays.begin(),
                               weekdays.end(),
                               [&time, use_curr_day, use_next_day](const auto &x) {
                                   return x.IsInRange(time, use_curr_day, use_next_day);
                               }))
               // .. and if month-day ranges are not specified or is in any month-day range
               && (monthdays.empty() ||
                   std::any_of(monthdays.begin(),
                               monthdays.end(),
                               [&time, use_curr_day, use_next_day](const auto &x) {
                                   return x.IsInRange(time, use_curr_day, use_next_day);
                               }));
    }

    std::vector<TimeSpan> times;
    std::vector<WeekdayRange> weekdays;
    std::vector<MonthdayRange> monthdays;
    Modifier modifier;
};

std::vector<OpeningHours> ParseOpeningHours(const std::string &str);

bool CheckOpeningHours(const std::vector<OpeningHours> &input, const struct tm &time);

} // namespace util
} // namespace osrm

#endif // OSRM_OPENING_HOURS_HPP

/*
    open source routing machine
    Copyright (C) Dennis Luxen, others 2010

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU AFFERO General Public License as published by
the Free Software Foundation; either version 3 of the License, or
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
or see http://www.gnu.org/licenses/agpl.txt.
*/

#ifndef TIMINGUTIL_H_
#define TIMINGUTIL_H_

// excluded as this requires boost 1.47 (for now)
// #include <boost/chrono.hpp>
// #include <boost/timer/timer.hpp>

// static boost::timer::cpu_timer my_timer;

// /** Returns a timestamp (now) in seconds (incl. a fractional part). */
// static inline double get_timestamp() {
//     boost::chrono::duration<double> duration = boost::chrono::nanoseconds(
//         my_timer.elapsed().user + my_timer.elapsed().system +
//         my_timer.elapsed().wall
//     );
//     return duration.count();
// }

#include <climits>
#include <cstdlib>


#ifdef _WIN32
#include <sys/timeb.h>
#include <sys/types.h>
#include <winsock.h>
void gettimeofday(struct timeval* t,void* timezone) {
    struct _timeb timebuffer;
    _ftime( &timebuffer );
    t->tv_sec=timebuffer.time;
    t->tv_usec=1000*timebuffer.millitm;
}
#else
#include <sys/time.h>
#endif

/** Returns a timestamp (now) in seconds (incl. a fractional part). */
static inline double get_timestamp() {
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return double(tp.tv_sec) + tp.tv_usec / 1000000.;
}

#endif /* TIMINGUTIL_H_ */

/*

Copyright (c) 2013, Project OSRM, Dennis Luxen, others
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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

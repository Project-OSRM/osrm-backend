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

#ifndef TIMEUTIL_H_
#define TIMEUTIL_H_

#include <climits>
#include <cmath>
#include <cstdlib>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifdef _WIN32
 #include <sys/timeb.h>
 #include <sys/types.h>
 #include <winsock.h>
 void gettimeofday(struct timeval* t,void* timezone)
 {       struct _timeb timebuffer;
        _ftime( &timebuffer );
        t->tv_sec=timebuffer.time;
        t->tv_usec=1000*timebuffer.millitm;
 }
#else
 #include <sys/time.h>
#endif

#ifdef _WIN32
 #include <boost/functional/hash.hpp>
#else
 #include <tr1/functional_hash.h>
#endif
#include <boost/thread.hpp>

/** Returns a timestamp (now) in seconds (incl. a fractional part). */
static inline double get_timestamp() {
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return double(tp.tv_sec) + tp.tv_usec / 1000000.;
}

static inline double y2lat(double a) { return 180/M_PI * (2 * atan(exp(a*M_PI/180)) - M_PI/2); }
static inline double lat2y(double a) { return 180/M_PI * log(tan(M_PI/4+a*(M_PI/180)/2)); }

static inline unsigned boost_thread_id_hash(boost::thread::id const& id) {
	std::stringstream ostr;
	ostr << id;
	std::tr1::hash<std::string> h;
	return h(ostr.str());
}

#endif /* TIMEUTIL_H_ */

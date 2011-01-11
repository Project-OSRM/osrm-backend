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
#include <sys/time.h>
#include <tr1/functional_hash.h>
#include <boost/thread.hpp>

/** Returns a timestamp (now) in seconds (incl. a fractional part). */
inline double get_timestamp()
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return double(tp.tv_sec) + tp.tv_usec / 1000000.;
}

double y2lat(double a) { return 180/M_PI * (2 * atan(exp(a*M_PI/180)) - M_PI/2); }
double lat2y(double a) { return 180/M_PI * log(tan(M_PI/4+a*(M_PI/180)/2)); }

inline unsigned boost_thread_id_hash(boost::thread::id const& id)
{
	std::stringstream ostr;
	ostr << id;
	std::tr1::hash<std::string> h;
	return h(ostr.str());
}

#endif /* TIMEUTIL_H_ */

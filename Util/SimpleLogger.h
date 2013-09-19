/*
    open source routing machine
    Copyright (C) Dennis Luxen, 2010

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


#ifndef SIMPLE_LOGGER_H_
#define SIMPLE_LOGGER_H_

#include <boost/assert.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>

#include <ostream>
#include <iostream>

enum LogLevel { logINFO, logWARNING, logDEBUG };
static	boost::mutex logger_mutex;

class LogPolicy : boost::noncopyable {
public:

	void Unmute() {
		m_is_mute = false;
	}

	void Mute() {
		m_is_mute = true;
	}

	bool IsMute() const {
		return m_is_mute;
	}

    static LogPolicy & GetInstance() {
	    static LogPolicy runningInstance;
    	return runningInstance;
    }

private:
	LogPolicy() : m_is_mute(true) { }
	bool m_is_mute;
};

class SimpleLogger {
public:
	SimpleLogger() : level(logINFO) { }

    std::ostringstream& Write(LogLevel l = logINFO) {
    	try {
			boost::mutex::scoped_lock lock(logger_mutex);
			level = l;
			os << "[";
			   	switch(level) {
				case logINFO:
		    		os << "info";
		    		break;
				case logWARNING:
		    		os << "warn";
		    		break;
				case logDEBUG:
#ifndef NDEBUG
		    		os << "debug";
#endif
		    		break;
				default:
		    		BOOST_ASSERT_MSG(false, "should not happen");
		    		break;
			}
			os << "] ";
		} catch (...) { }
	   	return os;
   }

	virtual ~SimpleLogger() {
		   	if(!LogPolicy::GetInstance().IsMute()) {
		   	switch(level) {
				case logINFO:
					std::cout << os.str() << std::endl;
					break;
				case logWARNING:
					std::cerr << os.str() << std::endl;
					break;
				case logDEBUG:
#ifndef NDEBUG
					std::cout << os.str() << std::endl;
#endif
					break;
				default:
					BOOST_ASSERT_MSG(false, "should not happen");
					break;
			}
	   	}
	}

private:
	LogLevel level;
	std::ostringstream os;
};

#endif /* SIMPLE_LOGGER_H_ */

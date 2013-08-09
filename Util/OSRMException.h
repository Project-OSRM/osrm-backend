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

#ifndef OSRM_EXCEPTION_H
#define OSRM_EXCEPTION_H

#include <exception>
#include <string>

class OSRMException: public std::exception {
public:
    OSRMException(const char * message) : message(message) {}
    OSRMException(const std::string & message) : message(message) {}
    virtual ~OSRMException() throw() {}
private:
    virtual const char* what() const throw() {
        return message.c_str();
    }
    const std::string message;
};

#endif /* OSRM_EXCEPTION_H */

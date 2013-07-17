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

#ifndef UUID_H
#define UUID_H

#include <boost/noncopyable.hpp>
#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.

#include <iostream>
#include <string>

//implements a singleton, i.e. there is one and only one conviguration object
class UUID : boost::noncopyable {
public:
	static UUID & GetInstance() {
		static UUID instance;
		return instance;
	}
	~UUID();
	const boost::uuids::uuid & GetUUID() const;
private:
    UUID();
	// initialize to {6ba7b810-9dad-11d1-80b4-00c04fd430c8}
    boost::uuids::uuid named_uuid;
};

#endif /* UUID_H */
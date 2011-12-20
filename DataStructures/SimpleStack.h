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

#ifndef SIMPLESTACK_H_
#define SIMPLESTACK_H_

#include <cassert>
#include <vector>

template<typename StackItemT, class ContainerT = std::vector<StackItemT> >
class SimpleStack {

private:
	int last;
	ContainerT arr;

public:
	SimpleStack() : last(-1) {
	}

	SimpleStack(std::size_t size_hint) : last(-1) {
		hint(size_hint);
	}

	inline void hint(std::size_t size_hint) {
		arr.reserve(size_hint);
	}

	inline void push(StackItemT t) {
		++last;
		arr.push_back(t);
	}

	inline void pop() {
		arr.pop_back();
		--last;
	}

	inline StackItemT top() {
		assert (last >= 0);
		return arr[last];
	}

	inline int size() {
		return last+1;
	}

	inline bool empty() {
		return (-1 == last);
	}
};


#endif /* SIMPLESTACK_H_ */

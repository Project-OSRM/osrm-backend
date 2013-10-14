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

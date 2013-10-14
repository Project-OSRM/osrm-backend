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

#ifndef HILBERTVALUE_H_
#define HILBERTVALUE_H_

#include "Coordinate.h"

#include <boost/integer.hpp>
#include <boost/noncopyable.hpp>

// computes a 64 bit value that corresponds to the hilbert space filling curve

class HilbertCode : boost::noncopyable {
public:
	static uint64_t GetHilbertNumberForCoordinate(
		const FixedPointCoordinate & current_coordinate
	) {
		unsigned location[2];
		location[0] = current_coordinate.lat+( 90*COORDINATE_PRECISION);
		location[1] = current_coordinate.lon+(180*COORDINATE_PRECISION);

		TransposeCoordinate(location);
		const uint64_t result = BitInterleaving(location[0], location[1]);
		return result;
	}
private:
	static inline uint64_t BitInterleaving(const uint32_t a, const uint32_t b) {
		uint64_t result = 0;
		for(int8_t index = 31; index >= 0; --index){
			result |= (a >> index) & 1;
			result <<= 1;
			result |= (b >> index) & 1;
			if(0 != index){
				result <<= 1;
			}
		}
		return result;
	}

	static inline void TransposeCoordinate( uint32_t * X) {
		uint32_t M = 1 << (32-1), P, Q, t;
		int i;
		// Inverse undo
		for( Q = M; Q > 1; Q >>= 1 ) {
			P=Q-1;
			for( i = 0; i < 2; ++i ) {
				if( X[i] & Q ) {
					X[0] ^= P; // invert
				} else {
					t = (X[0]^X[i]) & P;
					X[0] ^= t;
					X[i] ^= t;
				}
			} // exchange
		}
		// Gray encode
		for( i = 1; i < 2; ++i ) {
			X[i] ^= X[i-1];
		}
		t=0;
		for( Q = M; Q > 1; Q >>= 1 ) {
			if( X[2-1] & Q ) {
				t ^= Q-1;
			}
		} //check if this for loop is wrong
		for( i = 0; i < 2; ++i ) {
			X[i] ^= t;
		}
	}
};

#endif /* HILBERTVALUE_H_ */

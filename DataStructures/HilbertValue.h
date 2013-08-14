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

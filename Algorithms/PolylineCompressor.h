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

#ifndef POLYLINECOMPRESSOR_H_
#define POLYLINECOMPRESSOR_H_

#include <string>

#include "../DataStructures/ExtractorStructs.h"
#include "../DataStructures/SegmentInformation.h"
#include "../Util/StringUtil.h"

class PolylineCompressor {
private:
	inline void encodeVectorSignedNumber(std::vector<int> & numbers, std::string & output) {
		for(unsigned i = 0; i < numbers.size(); ++i) {
			numbers[i] <<= 1;
			if (numbers[i] < 0) {
				numbers[i] = ~(numbers[i]);
			}
		}
		for(unsigned i = 0; i < numbers.size(); ++i) {
			encodeNumber(numbers[i], output);
		}
	}

	inline void encodeNumber(int numberToEncode, std::string & output) {
		while (numberToEncode >= 0x20) {
			int nextValue = (0x20 | (numberToEncode & 0x1f)) + 63;
			output += (static_cast<char> (nextValue));
			if(92 == nextValue)
				output += (static_cast<char> (nextValue));
			numberToEncode >>= 5;
		}

		numberToEncode += 63;
		output += (static_cast<char> (numberToEncode));
		if(92 == numberToEncode)
			output += (static_cast<char> (numberToEncode));
	}

public:
    inline void printEncodedString(const std::vector<SegmentInformation>& polyline, std::string &output) {
    	std::vector<int> deltaNumbers;
        output += "\"";
        if(!polyline.empty()) {
            _Coordinate lastCoordinate = polyline[0].location;
            deltaNumbers.push_back( lastCoordinate.lat );
            deltaNumbers.push_back( lastCoordinate.lon );
            for(unsigned i = 1; i < polyline.size(); ++i) {
                if(!polyline[i].necessary)
                    continue;
                deltaNumbers.push_back(polyline[i].location.lat - lastCoordinate.lat);
                deltaNumbers.push_back(polyline[i].location.lon - lastCoordinate.lon);
                lastCoordinate = polyline[i].location;
            }
            encodeVectorSignedNumber(deltaNumbers, output);
        }
        output += "\"";

    }

	inline void printEncodedString(const std::vector<_Coordinate>& polyline, std::string &output) {
		std::vector<int> deltaNumbers(2*polyline.size());
		output += "\"";
		if(!polyline.empty()) {
			deltaNumbers[0] = polyline[0].lat;
			deltaNumbers[1] = polyline[0].lon;
			for(unsigned i = 1; i < polyline.size(); ++i) {
				deltaNumbers[(2*i)]   = (polyline[i].lat - polyline[i-1].lat);
				deltaNumbers[(2*i)+1] = (polyline[i].lon - polyline[i-1].lon);
			}
			encodeVectorSignedNumber(deltaNumbers, output);
		}
		output += "\"";
	}

    inline void printUnencodedString(std::vector<_Coordinate> & polyline, std::string & output) {
        output += "[";
        std::string tmp;
        for(unsigned i = 0; i < polyline.size(); i++) {
            convertInternalLatLonToString(polyline[i].lat, tmp);
            output += "[";
            output += tmp;
            convertInternalLatLonToString(polyline[i].lon, tmp);
            output += ", ";
            output += tmp;
            output += "]";
            if( i < polyline.size()-1 ) {
                output += ",";
            }
        }
        output += "]";
    }

    inline void printUnencodedString(std::vector<SegmentInformation> & polyline, std::string & output) {
        output += "[";
        std::string tmp;
        for(unsigned i = 0; i < polyline.size(); i++) {
            if(!polyline[i].necessary)
                continue;
            convertInternalLatLonToString(polyline[i].location.lat, tmp);
            output += "[";
            output += tmp;
            convertInternalLatLonToString(polyline[i].location.lon, tmp);
            output += ", ";
            output += tmp;
            output += "]";
            if( i < polyline.size()-1 ) {
                output += ",";
            }
        }
        output += "]";
    }
};

#endif /* POLYLINECOMPRESSOR_H_ */

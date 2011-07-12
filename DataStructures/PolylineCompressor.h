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

#include "ExtractorStructs.h"

class PolylineCompressor {
private:
    inline string encodeSignedNumber(int number) const {
        int signedNumber = number << 1;
        if (number < 0) {
            signedNumber = ~(signedNumber);
        }
        return (encodeNumber(signedNumber));
    }

    inline string encodeNumber(int numberToEncode) const {
        ostringstream encodeString;

        while (numberToEncode >= 0x20) {
            int nextValue = (0x20 | (numberToEncode & 0x1f)) + 63;
            encodeString << (static_cast<char> (nextValue));
            numberToEncode >>= 5;
        }

        numberToEncode += 63;
        encodeString << (static_cast<char> (numberToEncode));

        return encodeString.str();
    }

    inline void replaceBackslash(string & str) {
        size_t found = 0;
        do {
            found = str.find("\\", found);
            if(found ==string::npos)
                break;
            str.insert(found, "\\");
            found+=2;
        } while(true);
    }

public:
    inline void printEncodedString(vector<_Coordinate>& polyline, string &output) {
        output += "\"";
        if(!polyline.empty()) {
            output += encodeSignedNumber(polyline[0].lat);
            output += encodeSignedNumber(polyline[0].lon);
        }
        for(unsigned i = 1; i < polyline.size(); i++) {
            output += encodeSignedNumber(polyline[i].lat - polyline[i-1].lat);
            output += encodeSignedNumber(polyline[i].lon - polyline[i-1].lon);
        }
        output += "\"";
        replaceBackslash(output);
    }

    inline void printUnencodedString(vector<_Coordinate> & polyline, string & output) {
        output += "[";
        string tmp;
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
};

#endif /* POLYLINECOMPRESSOR_H_ */

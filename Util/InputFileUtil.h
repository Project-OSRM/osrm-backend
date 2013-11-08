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

#ifndef INPUTFILEUTIL_H_
#define INPUTFILEUTIL_H_

#include "SimpleLogger.h"
#include "../typedefs.h"

#include <boost/filesystem.hpp>

// Check if file exists and if it can be opened for reading with ifstream an object
inline bool testDataFile(const std::string & filename){
    boost::filesystem::path fileToTest(filename);
	if(!boost::filesystem::exists(fileToTest))       {
		SimpleLogger().Write(logWARNING) <<
			"Failed to open file " << filename << " for reading.";

		return false;
	}
	return true;
}

inline bool testDataFiles(int argc, char *argv[]){
	for(int i = 0; i < argc; ++i) {
		if(!testDataFile(argv[i]))
			return false;
	}
	return true;
}

#endif /* INPUTFILEUTIL_H_ */

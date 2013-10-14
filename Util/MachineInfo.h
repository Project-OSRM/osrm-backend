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

#ifndef MACHINE_INFO_H
#define MACHINE_INFO_H

#if defined(__APPLE__) || defined(__FreeBSD__)
extern "C" {
    #include <sys/types.h>
    #include <sys/sysctl.h>
}
#elif defined _WIN32
    #include <windows.h>
#endif

enum Endianness {
    LittleEndian = 1,
    BigEndian = 2
};

//Function is optimized to a single 'mov eax,1' on GCC, clang and icc using -O3
inline Endianness getMachineEndianness() {
    int i(1);
    char *p = (char *) &i;
    if (1 == p[0])
        return LittleEndian;
    return BigEndian;
}

// Reverses Network Byte Order into something usable, compiles down to a bswap-mov combination
inline unsigned swapEndian(unsigned x) {
    if(getMachineEndianness() == LittleEndian)
        return ( (x>>24) | ((x<<8) & 0x00FF0000) | ((x>>8) & 0x0000FF00) | (x<<24) );
    return x;
}

// Returns the physical memory size in kilobytes
inline unsigned GetPhysicalmemory(void){
#if defined(SUN5) || defined(__linux__)
	return (sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGESIZE));

#elif defined(__APPLE__)
	int mib[2] = {CTL_HW, HW_MEMSIZE};
	long long memsize;
	size_t len = sizeof(memsize);
	sysctl(mib, 2, &memsize, &len, NULL, 0);
	return memsize/1024;

#elif defined(__FreeBSD__)
	int mib[2] = {CTL_HW, HW_PHYSMEM};
	long long memsize;
	size_t len = sizeof(memsize);
	sysctl(mib, 2, &memsize, &len, NULL, 0);
	return memsize/1024;

#elif defined(_WIN32)
	MEMORYSTATUSEX status;
	status.dwLength = sizeof(status);
	GlobalMemoryStatusEx(&status);
	return status.ullTotalPhys/1024;
#else
	std::cout << "[Warning] Compiling on unknown architecture." << std::endl
		<< "Please file a ticket at http://project-osrm.org" << std::endl;
	return 2048*1024; /* 128 Mb default memory */

#endif
}
#endif // MACHINE_INFO_H


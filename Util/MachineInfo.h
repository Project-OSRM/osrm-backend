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

/* Returns the physical memory size in kilobytes */
unsigned GetPhysicalmemory(void){
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
#endif

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

#ifndef _OPENMPREPLACEMENTY_H
#define _OPENMPREPLACEMENTY_H

#ifdef _OPENMP
#include <omp.h>  	
#else
inline const int omp_get_num_procs() { return 1; }
inline const int omp_get_max_threads() { return 1; }
inline const int omp_get_thread_num() { return 0; }
inline const void omp_set_num_threads(int i) {}
#endif

#endif

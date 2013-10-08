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

#ifndef SHARED_MEMORY_FACTORY_H
#define SHARED_MEMORY_FACTORY_H

#include "../Util/OSRMException.h"
#include "../Util/SimpleLogger.h"

#include <boost/noncopyable.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/xsi_shared_memory.hpp>

#include <algorithm>
#include <exception>

struct OSRMLockFile {
	boost::filesystem::path operator()() {
		boost::filesystem::path temp_dir =
			boost::filesystem::temp_directory_path();
	    boost::filesystem::path lock_file = temp_dir / "osrm.lock";
	    return lock_file;
	}
};

class SharedMemory : boost::noncopyable {

	//Remove shared memory on destruction
	class shm_remove : boost::noncopyable {
	private:
		int m_shmid;
		bool m_initialized;
	public:
		void SetID(int shmid) {
		 	m_shmid = shmid;
		 	m_initialized = true;
		}

		shm_remove() : m_shmid(INT_MIN), m_initialized(false) {}

		~shm_remove(){
			if(m_initialized) {
				SimpleLogger().Write(logDEBUG) <<
					"automatic memory deallocation";
				boost::interprocess::xsi_shared_memory::remove(m_shmid);
			}
		}
	};

public:
		void * Ptr() const {
			return region.get_address();
		}

		template<typename IdentifierT >
		SharedMemory(
			const boost::filesystem::path & lock_file,
			const IdentifierT id,
			const unsigned size = 0
		) : key(
				lock_file.string().c_str(),
				id
		) {
	    	if( 0 == size ){ //read_only
	    		shm = boost::interprocess::xsi_shared_memory (
	    			boost::interprocess::open_only,
	    			key
				);
	    		region = boost::interprocess::mapped_region (
	    			shm,
	    			boost::interprocess::read_only
				);
	    	} else { //writeable pointer
	    		//remove previously allocated mem
	    		RemoveSharedMemory(key);
	    		shm = boost::interprocess::xsi_shared_memory (
	    			boost::interprocess::create_only,
	    			key,
	    			size
	    		);
			    region = boost::interprocess::mapped_region (
			    	shm,
			    	boost::interprocess::read_write
		    	);

     			remover.SetID( shm.get_shmid() );
     			SimpleLogger().Write(logDEBUG) <<
     				"writeable memory allocated " << size << " bytes";
	    	}
		}

	template<typename IdentifierT >
	static bool RegionExists(
		const boost::filesystem::path & lock_file,
		const IdentifierT id
	) {
		boost::interprocess::xsi_key key( lock_file.string().c_str(), id );
		return RegionExists(key);
	}

private:
	static bool RegionExists( const boost::interprocess::xsi_key &key ) {
		bool result = true;
	    try {
		    boost::interprocess::xsi_shared_memory shm(
		        boost::interprocess::open_only,
		        key
		    );
	    } catch(...) {
	    	result = false;
	    }
	    return result;
	}

	static void RemoveSharedMemory(
		const boost::interprocess::xsi_key &key
	) {
		try{
			SimpleLogger().Write(logDEBUG) << "deallocating prev memory";
			boost::interprocess::xsi_shared_memory xsi(
				boost::interprocess::open_only,
				key
			);
			boost::interprocess::xsi_shared_memory::remove(xsi.get_shmid());
		} catch(boost::interprocess::interprocess_exception &e){
			if(e.get_error_code() != boost::interprocess::not_found_error) {
				throw;
			}
		}
	}

	boost::interprocess::xsi_key key;
	boost::interprocess::xsi_shared_memory shm;
	boost::interprocess::mapped_region region;
	shm_remove remover;
};

template<class LockFileT = OSRMLockFile>
class SharedMemoryFactory_tmpl : boost::noncopyable {
public:
	template<typename IdentifierT >
	static SharedMemory * Get(
		const IdentifierT & id,
		const unsigned size = 0
	) {
		try {
			LockFileT lock_file;
		    if(!boost::filesystem::exists(lock_file()) ) {
		    	if( 0 == size ) {
	      			throw OSRMException("lock file does not exist, exiting");
	      		} else {
	      			boost::filesystem::ofstream ofs(lock_file());
	      			ofs.close();
	      		}
	      	}
			return new SharedMemory(lock_file(), id, size);
	   	} catch(const boost::interprocess::interprocess_exception &e){
    		SimpleLogger().Write(logWARNING) <<
    			"caught exception: " << e.what() <<
    			", code " << e.get_error_code();
    		throw OSRMException(e.what());
    	}
	}

private:
	SharedMemoryFactory_tmpl() {}
};

typedef SharedMemoryFactory_tmpl<> SharedMemoryFactory;

#endif /* SHARED_MEMORY_POINTER_FACTORY_H */

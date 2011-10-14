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

#ifndef CONCURRENTQUEUE_H_INCLUDED
#define CONCURRENTQUEUE_H_INCLUDED

#include "typedefs.h"

/* 
   Concurrent Queue written by Anthony Williams: 
   http://www.justsoftwaresolutions.co.uk/threading/implementing-a-thread-safe-queue-using-condition-variables.html  
*/
template<typename Data>
class concurrent_queue
{
private:
    std::queue<Data> internal_queue;
    mutable boost::mutex queue_mutex;
    mutable boost::mutex queue_full_mutex;
    boost::condition_variable queue_cv;
    boost::condition_variable queue_full_cv;
    const size_t max_queue_size;

    bool size_exceeded() const {
        boost::mutex::scoped_lock lock(queue_mutex);
        return internal_queue.size() >= max_queue_size;
    }

public:
    concurrent_queue(const size_t max_size) 
        : max_queue_size(max_size) {
    }

    void push(Data const& data)
    {
        if (size_exceeded()) {
            boost::mutex::scoped_lock qf_lock(queue_full_mutex);
            queue_full_cv.wait(qf_lock);
        }

        boost::mutex::scoped_lock lock(queue_mutex);
        internal_queue.push(data);
        lock.unlock();
        queue_cv.notify_one();
    }

    bool empty() const
    {
        boost::mutex::scoped_lock lock(queue_mutex);
        return internal_queue.empty();
    }

    bool try_pop(Data& popped_value)
    {
        boost::mutex::scoped_lock lock(queue_mutex);
        if(internal_queue.empty())
        {
            return false;
        }
        
        popped_value=internal_queue.front();
        internal_queue.pop();
        queue_full_cv.notify_one();
        return true;
    }

    void wait_and_pop(Data& popped_value)
    {
        boost::mutex::scoped_lock lock(queue_mutex);
        while(internal_queue.empty())
        {
            queue_cv.wait(lock);
        }
        
        popped_value=internal_queue.front();
        internal_queue.pop();
        queue_full_cv.notify_one();
    }

    int size() const {
        boost::mutex::scoped_lock lock(queue_mutex);
        return static_cast<int>(internal_queue.size());
    }
};

#endif //#ifndef CONCURRENTQUEUE_H_INCLUDED
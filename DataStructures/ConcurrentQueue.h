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

#include <boost/bind.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

#include "../typedefs.h"

template<typename Data>
class ConcurrentQueue {

    typedef typename boost::circular_buffer<Data>::size_type size_t;

public:
    ConcurrentQueue(const size_t max_size) : internal_queue(max_size) { }

    void push(Data const& data) {
        boost::mutex::scoped_lock lock(m_mutex);
        m_not_full.wait(lock, boost::bind(&ConcurrentQueue<Data>::is_not_full, this));
        internal_queue.push_back(data);
        lock.unlock();
        m_not_empty.notify_one();
    }

    bool empty() const {
        return internal_queue.empty();
    }

    void wait_and_pop(Data& popped_value) {
        boost::mutex::scoped_lock lock(m_mutex);
        m_not_empty.wait(lock, boost::bind(&ConcurrentQueue<Data>::is_not_empty, this));
        popped_value=internal_queue.front();
        internal_queue.pop_front();
        lock.unlock();
        m_not_full.notify_one();
    }

    bool try_pop(Data& popped_value) {
        boost::mutex::scoped_lock lock(m_mutex);
        if(internal_queue.empty()) {
            return false;
        }
        popped_value=internal_queue.front();
        internal_queue.pop_front();
        lock.unlock();
        m_not_full.notify_one();
        return true;
    }

private:
    boost::circular_buffer<Data> internal_queue;
    boost::mutex m_mutex;
    boost::condition m_not_empty;
    boost::condition m_not_full;

    inline bool is_not_empty() const { return internal_queue.size() > 0; }
    inline bool is_not_full() const { return internal_queue.size() < internal_queue.capacity(); }
};

#endif //#ifndef CONCURRENTQUEUE_H_INCLUDED

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

#ifndef CONCURRENTQUEUE_H_
#define CONCURRENTQUEUE_H_

#include "../typedefs.h"

#include <boost/bind.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

template<typename Data>
class ConcurrentQueue {

public:
    ConcurrentQueue(const size_t max_size) : m_internal_queue(max_size) { }

    inline void push(const Data & data) {
        boost::mutex::scoped_lock lock(m_mutex);
        m_not_full.wait(
            lock,
            boost::bind(&ConcurrentQueue<Data>::is_not_full, this)
        );
        m_internal_queue.push_back(data);
        lock.unlock();
        m_not_empty.notify_one();
    }

    inline bool empty() const {
        return m_internal_queue.empty();
    }

    inline void wait_and_pop(Data & popped_value) {
        boost::mutex::scoped_lock lock(m_mutex);
        m_not_empty.wait(
            lock,
            boost::bind(&ConcurrentQueue<Data>::is_not_empty, this)
        );
        popped_value = m_internal_queue.front();
        m_internal_queue.pop_front();
        lock.unlock();
        m_not_full.notify_one();
    }

    inline bool try_pop(Data& popped_value) {
        boost::mutex::scoped_lock lock(m_mutex);
        if(m_internal_queue.empty()) {
            return false;
        }
        popped_value=m_internal_queue.front();
        m_internal_queue.pop_front();
        lock.unlock();
        m_not_full.notify_one();
        return true;
    }

private:
    inline bool is_not_empty() const {
        return !m_internal_queue.empty();
    }

    inline bool is_not_full() const {
        return m_internal_queue.size() < m_internal_queue.capacity();
    }

    boost::circular_buffer<Data>    m_internal_queue;
    boost::mutex                    m_mutex;
    boost::condition                m_not_empty;
    boost::condition                m_not_full;
};

#endif /* CONCURRENTQUEUE_H_ */

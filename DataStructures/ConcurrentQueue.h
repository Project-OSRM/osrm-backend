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

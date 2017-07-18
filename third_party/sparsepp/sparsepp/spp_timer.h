/**
   Copyright (c) 2016 Mariano Gonzalez

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/

#ifndef spp_timer_h_guard
#define spp_timer_h_guard

#include <chrono>

namespace spp
{
    template<typename time_unit = std::milli>
    class Timer 
    {
    public:
        Timer()                 { reset(); }
        void reset()            { _start = _snap = clock::now();  }
        void snap()             { _snap = clock::now();  }

        float get_total() const { return get_diff<float>(_start, clock::now()); }
        float get_delta() const { return get_diff<float>(_snap, clock::now());  }
        
    private:
        using clock = std::chrono::high_resolution_clock;
        using point = std::chrono::time_point<clock>;

        template<typename T>
        static T get_diff(const point& start, const point& end) 
        {
            using duration_t = std::chrono::duration<T, time_unit>;

            return std::chrono::duration_cast<duration_t>(end - start).count();
        }

        point _start;
        point _snap;
    };
}

#endif // spp_timer_h_guard

#include <map>
#include <unordered_map>
#include <string>
#include <iostream>
#include <chrono>
#include <vector>
#include <sparsepp/spp.h>

#include <sstream>

namespace patch
{
    template <typename T> std::string to_string(const T& n)
    {
        std::ostringstream stm;
        stm << n;
        return stm.str();
    }
}

#if defined(SPP_NO_CXX11_RVALUE_REFERENCES)
    #warning "problem: we expect spp will detect we have rvalue support"
#endif

template <typename T>
using milliseconds = std::chrono::duration<T, std::milli>;

class custom_type
{
    std::string one = "one";
    std::string two = "two";
    std::uint32_t three = 3;
    std::uint64_t four = 4;
    std::uint64_t five = 5;
public:
    custom_type() = default;
    // Make object movable and non-copyable
    custom_type(custom_type &&) = default;
    custom_type& operator=(custom_type &&) = default;
    // should be automatically deleted per http://www.slideshare.net/ripplelabs/howard-hinnant-accu2014
    //custom_type(custom_type const&) = delete;
    //custom_type& operator=(custom_type const&) = delete;
};

void test(std::size_t iterations, std::size_t container_size)
{
    std::clog << "bench: iterations: " << iterations <<  " / container_size: "  << container_size << "\n";
    {
        std::size_t count = 0;
        auto t1 = std::chrono::high_resolution_clock::now();
        for (std::size_t i=0; i<iterations; ++i)
        {
            std::unordered_map<std::string,custom_type> m;
            m.reserve(container_size);
            for (std::size_t j=0; j<container_size; ++j)
                m.emplace(patch::to_string(j),custom_type());
            count += m.size();
        }
        auto t2 = std::chrono::high_resolution_clock::now();
        auto elapsed = milliseconds<double>(t2 - t1).count();
        if (count != iterations*container_size)
            std::clog << "  invalid count: " << count << "\n";
        std::clog << "  std::unordered_map:     " << std::fixed << int(elapsed) << " ms\n";
    }

    {
        std::size_t count = 0;
        auto t1 = std::chrono::high_resolution_clock::now();
        for (std::size_t i=0; i<iterations; ++i)
        {
            std::map<std::string,custom_type> m;
            for (std::size_t j=0; j<container_size; ++j)
                m.emplace(patch::to_string(j),custom_type());
            count += m.size();
        }
        auto t2 = std::chrono::high_resolution_clock::now();
        auto elapsed = milliseconds<double>(t2 - t1).count();
        if (count != iterations*container_size)
            std::clog << "  invalid count: " << count << "\n";
        std::clog << "  std::map:               " << std::fixed << int(elapsed) << " ms\n";
    }

    {
        std::size_t count = 0;
        auto t1 = std::chrono::high_resolution_clock::now();
        for (std::size_t i=0; i<iterations; ++i)
        {
            std::vector<std::pair<std::string,custom_type>> m;
            m.reserve(container_size);
            for (std::size_t j=0; j<container_size; ++j)
                m.emplace_back(patch::to_string(j),custom_type());
            count += m.size();
        }
        auto t2 = std::chrono::high_resolution_clock::now();
        auto elapsed = milliseconds<double>(t2 - t1).count();
        if (count != iterations*container_size)
            std::clog << "  invalid count: " << count << "\n";
        std::clog << "  std::vector<std::pair>: " << std::fixed << int(elapsed) << " ms\n";
    }

    {
        std::size_t count = 0;
        auto t1 = std::chrono::high_resolution_clock::now();
        for (std::size_t i=0; i<iterations; ++i)
        {
            spp::sparse_hash_map<std::string,custom_type> m;
            m.reserve(container_size);
            for (std::size_t j=0; j<container_size; ++j)
                m.emplace(patch::to_string(j),custom_type());
            count += m.size();
        }
        auto t2 = std::chrono::high_resolution_clock::now();
        auto elapsed = milliseconds<double>(t2 - t1).count();
        if (count != iterations*container_size)
            std::clog << "  invalid count: " << count << "\n";
        std::clog << "  spp::sparse_hash_map:   " << std::fixed << int(elapsed) << " ms\n";
    }

}

int main()
{
    std::size_t iterations = 100000;

    test(iterations,1);
    test(iterations,10);
    test(iterations,50);
}

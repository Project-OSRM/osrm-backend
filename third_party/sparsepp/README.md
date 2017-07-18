[![Build Status](https://travis-ci.org/greg7mdp/sparsepp.svg?branch=master)](https://travis-ci.org/greg7mdp/sparsepp)

# Sparsepp: A fast, memory efficient hash map for C++

Sparsepp is derived from Google's excellent [sparsehash](https://github.com/sparsehash/sparsehash) implementation. It aims to achieve the following objectives:

- A drop-in alternative for unordered_map and unordered_set.
- **Extremely low memory usage** (typically about one byte overhead per entry).
- **Very efficient**, typically faster than your compiler's unordered map/set or Boost's.
- **C++11 support** (if supported by compiler).
- ~~Single header~~ not anymore
- **Tested** on Windows (vs2010-2015, g++), linux (g++, clang++) and MacOS (clang++).

We believe Sparsepp provides an unparalleled combination of performance and memory usage, and will outperform your compiler's unordered_map on both counts. Only Google's `dense_hash_map` is consistently faster, at the cost of much greater memory usage (especially when the final size of the map is not known in advance). 

For a detailed comparison of various hash implementations, including Sparsepp, please see our [write-up](bench.md).

## Example

```c++
#include <iostream>
#include <string>
#include <sparsepp/spp.h>

using spp::sparse_hash_map;
 
int main()
{
    // Create an unordered_map of three strings (that map to strings)
    sparse_hash_map<std::string, std::string> email = 
    {
        { "tom",  "tom@gmail.com"},
        { "jeff", "jk@gmail.com"},
        { "jim",  "jimg@microsoft.com"}
    };
 
    // Iterate and print keys and values 
    for (const auto& n : email) 
        std::cout << n.first << "'s email is: " << n.second << "\n";
 
    // Add a new entry
    email["bill"] = "bg@whatever.com";
 
    // and print it
    std::cout << "bill's email is: " << email["bill"] << "\n";
 
    return 0;
}
```

## Installation

No compilation is needed, as this is a header-only library. The installation consist in copying the sparsepp directory wherever it will be convenient to include in your project(s). Also make the path to this directory is provided to the compiler with the `-I` option.

## Warning - iterator invalidation on erase/insert

1. erasing elements is likely to invalidate iterators (for example when calling `erase()`)

2. inserting new elements is likely to invalidate iterators (iterator invalidation can also happen with std::unordered_map if rehashing occurs due to the insertion)

## Usage

As shown in the example above, you need to include the header file: `#include <sparsepp/spp.h>`

This provides the implementation for the following classes:

```c++
namespace spp
{
    template <class Key, 
              class T,
              class HashFcn  = spp_hash<Key>,  
              class EqualKey = std::equal_to<Key>,
              class Alloc    = libc_allocator_with_realloc<std::pair<const Key, T>>>
    class sparse_hash_map;

    template <class Value,
              class HashFcn  = spp_hash<Value>,
              class EqualKey = std::equal_to<Value>,
              class Alloc    = libc_allocator_with_realloc<Value>>
    class sparse_hash_set;
};
```

These classes provide the same interface as std::unordered_map and std::unordered_set, with the following differences:

- Calls to `erase()` may invalidate iterators. However, conformant to the C++11 standard, the position and range erase functions return an iterator pointing to the position immediately following the last of the elements erased. This makes it easy to traverse a sparse hash table and delete elements matching a condition. For example to delete odd values:
   
   ```c++
   for (auto it = c.begin(); it != c.end(); )
       if (it->first % 2 == 1)
          it = c.erase(it);
       else
          ++it;
   ```
   
   As for std::unordered_map, the order of the elements that are not erased is preserved.

- Since items are not grouped into buckets, Bucket APIs have been adapted: `max_bucket_count` is equivalent to `max_size`, and `bucket_count` returns the sparsetable size, which is normally at least twice the number of items inserted into the hash_map.

## Memory allocator on Windows (when building with Visual Studio)

When building with the Microsoft compiler, we provide a custom allocator because the default one (from the Visual C++ runtime) fragments memory when reallocating. 

This is desirable *only* when creating large sparsepp hash maps. If you create lots of small hash_maps, memory usage may increase instead of decreasing as expected.  The reason is that, for each instance of a hash_map, the custom memory allocator creates a new memory space to allocate from, which is typically 4K, so it may be a big waste if just a few items are allocated.

In order to use the custom spp allocator, define the following preprocessor variable before including `<spp/spp.h>`:

`#define SPP_USE_SPP_ALLOC 1`

## Integer keys, and other hash function considerations.

1. For basic integer types, sparsepp provides a default hash function which does some mixing of the bits of the keys (see [Integer Hashing](http://burtleburtle.net/bob/hash/integer.html)). This prevents a pathological case where inserted keys are sequential (1, 2, 3, 4, ...), and the lookup on non-present keys becomes very slow. 

   Of course, the user of sparsepp may provide its own hash function,  as shown below:
   
   ```c++
   #include <sparsepp/spp.h>
   
   struct Hash64 {
       size_t operator()(uint64_t k) const { return (k ^ 14695981039346656037ULL) * 1099511628211ULL; }
   };
   
   struct Hash32 {
       size_t operator()(uint32_t k) const { return (k ^ 2166136261U)  * 16777619UL; }
   };
   
   int main() 
   {
       spp::sparse_hash_map<uint64_t, double, Hash64> map;
       ...
   }
   
   ```

2. When the user provides its own hash function, for example when inserting custom classes into a hash map, sometimes the resulting hash keys have similar low order bits and cause many collisions, decreasing the efficiency of the hash map. To address this use case, sparsepp provides an optional 'mixing' of the hash key (see [Integer Hash Function](https://gist.github.com/badboy/6267743) which can be enabled by defining the proprocessor macro: SPP_MIX_HASH. 

## Example 2 - providing a hash function for a user-defined class

In order to use a sparse_hash_set or sparse_hash_map, a hash function should be provided. Even though a the hash function can be provided via the HashFcn template parameter, we recommend injecting a specialization of `std::hash` for the class into the "std" namespace. For example:

```c++
#include <iostream>
#include <functional>
#include <string>
#include <sparsepp/spp.h>

using std::string;

struct Person 
{
    bool operator==(const Person &o) const 
    { return _first == o._first && _last == o._last; }

    string _first;
    string _last;
};

namespace std
{
    // inject specialization of std::hash for Person into namespace std
    // ----------------------------------------------------------------
    template<> 
    struct hash<Person>
    {
        std::size_t operator()(Person const &p) const
        {
            std::size_t seed = 0;
            spp::hash_combine(seed, p._first);
            spp::hash_combine(seed, p._last);
            return seed;
        }
    };
}
 
int main()
{
    // As we have defined a specialization of std::hash() for Person, 
    // we can now create sparse_hash_set or sparse_hash_map of Persons
    // ----------------------------------------------------------------
    spp::sparse_hash_set<Person> persons = { { "John", "Galt" }, 
                                             { "Jane", "Doe" } };
    for (auto& p: persons)
        std::cout << p._first << ' ' << p._last << '\n';
}
```

The `std::hash` specialization for `Person` combines the hash values for both first and last name using the convenient spp::hash_combine function, and returns the combined hash value. 

spp::hash_combine is provided by the header `sparsepp/spp.h`. However, class definitions often appear in header files, and it is desirable to limit the size of headers included in such header files, so we provide the very small header `sparsepp/spp_utils.h` for that purpose:

```c++
#include <string>
#include <sparsepp/spp_utils.h>

using std::string;
 
struct Person 
{
    bool operator==(const Person &o) const 
    { 
        return _first == o._first && _last == o._last && _age == o._age; 
    }

    string _first;
    string _last;
    int    _age;
};

namespace std
{
    // inject specialization of std::hash for Person into namespace std
    // ----------------------------------------------------------------
    template<> 
    struct hash<Person>
    {
        std::size_t operator()(Person const &p) const
        {
            std::size_t seed = 0;
            spp::hash_combine(seed, p._first);
            spp::hash_combine(seed, p._last);
            spp::hash_combine(seed, p._age);
            return seed;
        }
    };
}
```

## Example 3 - serialization

sparse_hash_set and sparse_hash_map can easily be serialized/unserialized to a file or network connection.
This support is implemented in the following APIs:

```c++
    template <typename Serializer, typename OUTPUT>
    bool serialize(Serializer serializer, OUTPUT *stream);

    template <typename Serializer, typename INPUT>
    bool unserialize(Serializer serializer, INPUT *stream);
```

The following example demonstrates how a simple sparse_hash_map can be written to a file, and then read back. The serializer we use read and writes to a file using the stdio APIs, but it would be equally simple to write a serialized using the stream APIS:

```c++
#include <cstdio>

#include <sparsepp/spp.h>

using spp::sparse_hash_map;
using namespace std;

class FileSerializer 
{
public:
    // serialize basic types to FILE
    // -----------------------------
    template <class T>
    bool operator()(FILE *fp, const T& value) 
    {
        return fwrite((const void *)&value, sizeof(value), 1, fp) == 1;
    }

    template <class T>
    bool operator()(FILE *fp, T* value) 
    {
        return fread((void *)value, sizeof(*value), 1, fp) == 1;
    }

    // serialize std::string to FILE
    // -----------------------------
    bool operator()(FILE *fp, const string& value) 
    {
        const size_t size = value.size();
        return (*this)(fp, size) && fwrite(value.c_str(), size, 1, fp) == 1;
    }

    bool operator()(FILE *fp, string* value) 
    {
        size_t size;
        if (!(*this)(fp, &size)) 
            return false;
        char* buf = new char[size];
        if (fread(buf, size, 1, fp) != 1) 
        {
            delete [] buf;
            return false;
        }
        new (value) string(buf, (size_t)size);
        delete[] buf;
        return true;
    }

    // serialize std::pair<const A, B> to FILE - needed for maps
    // ---------------------------------------------------------
    template <class A, class B>
    bool operator()(FILE *fp, const std::pair<const A, B>& value)
    {
        return (*this)(fp, value.first) && (*this)(fp, value.second);
    }

    template <class A, class B>
    bool operator()(FILE *fp, std::pair<const A, B> *value) 
    {
        return (*this)(fp, (A *)&value->first) && (*this)(fp, &value->second);
    }
};

int main(int argc, char* argv[]) 
{
    sparse_hash_map<string, int> age{ { "John", 12 }, {"Jane", 13 }, { "Fred", 8 } };

    // serialize age hash_map to "ages.dmp" file
    FILE *out = fopen("ages.dmp", "wb");
    age.serialize(FileSerializer(), out);
    fclose(out);

    sparse_hash_map<string, int> age_read;

    // read from "ages.dmp" file into age_read hash_map 
    FILE *input = fopen("ages.dmp", "rb");
    age_read.unserialize(FileSerializer(), input);
    fclose(input);

    // print out contents of age_read to verify correct serialization
    for (auto& v : age_read)
        printf("age_read: %s -> %d\n", v.first.c_str(), v.second);
}
```

## Thread safety

Sparsepp follows the thread safety rules of the Standard C++ library. In Particular:

- A single sparsepp hash table is thread safe for reading from multiple threads. For example, given a hash table A, it is safe to read A from thread 1 and from thread 2 simultaneously.

- If a single hash table is being written to by one thread, then all reads and writes to that hash table on the same or other threads must be protected. For example, given a hash table A, if thread 1 is writing to A, then thread 2 must be prevented from reading from or writing to A.

- It is safe to read and write to one instance of a type even if another thread is reading or writing to a different instance of the same type. For example, given hash tables A and B of the same type, it is safe if A is being written in thread 1 and B is being read in thread 2.

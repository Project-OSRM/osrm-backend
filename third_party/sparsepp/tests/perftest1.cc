// compile on linux with: g++ -std=c++11 -O2 perftest1.cc -o perftest1
// -----------------------------------------------------------------------
#include <fstream>
#include <iostream>
#include <ctime>
#include <cstdio>
#include <climits>
#include <functional>
#include <vector>
#include <utility>

#include <sparsepp/spp_timer.h>

#define SPP 1
#define DENSE 0
#define SPARSE 0
#define STD 0

#if SPP
    #include <sparsepp/spp.h>
#elif DENSE
    #include <google/dense_hash_map>
#elif SPARSE
    #include <google/sparse_hash_map>
#elif STD
    #include <unordered_map>
#endif

using std::make_pair;

template <class T>
void test(T &s, int count) 
{
    spp::Timer<std::milli> timer;

    timer.snap();
    srand(0);
    for (int i = 0; i < count; ++i) 
        s.insert(make_pair(rand(), i));

    printf("%d random inserts         in %5.2f seconds\n", count, timer.get_delta() / 1000);

    timer.snap();
    srand(0);
    for (int i = 0; i < count; ++i)
        s.find(rand()); 

    printf("%d random finds           in %5.2f seconds\n", count, timer.get_delta() / 1000);

    timer.snap();
    srand(1);
    for (int i = 0; i < count; ++i)
        s.find(rand());
    printf("%d random not-finds       in %5.2f seconds\n", count, timer.get_delta() / 1000);

    s.clear();
    timer.snap();
    srand(0);
    for (int i = 0; i < count; ++i) 
        s.insert(make_pair(i, i));
    printf("%d sequential inserts     in %5.2f seconds\n", count, timer.get_delta() / 1000);

    timer.snap();
    srand(0);
    for (int i = 0; i < count; ++i)
        s.find(i);

    printf("%d sequential finds       in %5.2f seconds\n", count, timer.get_delta() / 1000);

    timer.snap();
    srand(1);
    for (int i = 0; i < count; ++i) 
    { 
        int x = rand();
        s.find(x);
    }
    printf("%d random not-finds       in %5.2f seconds\n", count, timer.get_delta() / 1000);

    s.clear();
    timer.snap();
    srand(0);
    for (int i = 0; i < count; ++i) 
        s.insert(make_pair(-i, -i));

    printf("%d neg sequential inserts in %5.2f seconds\n", count, timer.get_delta() / 1000);

    timer.snap();
    srand(0);
    for (int i = 0; i < count; ++i)
        s.find(-i);

    printf("%d neg sequential finds   in %5.2f seconds\n", count, timer.get_delta() / 1000);

    timer.snap();
    srand(1);
    for (int i = 0; i < count; ++i) 
        s.find(rand());
    printf("%d random not-finds       in %5.2f seconds\n", count, timer.get_delta() / 1000);

    s.clear();    
}


struct Hasher64 {
    size_t operator()(uint64_t k) const { return (k ^ 14695981039346656037ULL) * 1099511628211ULL; }
};

struct Hasher32 {
    size_t operator()(uint32_t k) const { return (k ^ 2166136261U)  * 16777619UL; }
};

struct Hasheri32 {
    size_t operator()(int k) const 
    {
        return (k ^ 2166136261U)  * 16777619UL; 
    }
};

struct Hasher_32 {
    size_t operator()(int k) const 
    {
        uint32_t a = (uint32_t)k;
#if 0
        a = (a ^ 61) ^ (a >> 16);
        a = a + (a << 3);
        a = a ^ (a >> 4);
        a = a * 0x27d4eb2d;
        a = a ^ (a >> 15);
        return a;
#else
        a = a ^ (a >> 4);
        a = (a ^ 0xdeadbeef) + (a << 5);
        a = a ^ (a >> 11);
        return a;
#endif
    }
};

int main() 
{
#if SPP
    spp::sparse_hash_map<int, int /*, Hasheri32 */> s;
    printf ("Testing spp::sparse_hash_map\n");
#elif DENSE
    google::dense_hash_map<int, int/* , Hasher_32 */> s;
    s.set_empty_key(-INT_MAX); 
    s.set_deleted_key(-(INT_MAX - 1));
    printf ("Testing google::dense_hash_map\n");
#elif SPARSE
    google::sparse_hash_map<int, int/* , Hasher_32 */> s;
    s.set_deleted_key(-INT_MAX); 
    printf ("Testing google::sparse_hash_map\n");
#elif STD
    std::unordered_map<int, int/* , Hasher_32 */> s;
    printf ("Testing std::unordered_map\n");
#endif
    printf ("------------------------------\n");
    test(s, 50000000);


    return 0;
}

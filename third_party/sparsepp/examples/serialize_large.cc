#include <cstdio>
#include <stdlib.h>
#include <algorithm>
#include <vector>
#include <sparsepp/spp_timer.h>
#include <sparsepp/spp_memory.h>
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

float _to_gb(uint64_t m) { return (float)((double)m / (1024 * 1024 * 1024)); }

int main(int, char* [])
{
    sparse_hash_map<string, int> age;

    for (size_t i=0; i<10000000; ++i)
    {
        char buff[20];
        sprintf(buff, "%zu", i);
        age.insert(std::make_pair(std::string(buff), i));
    }

    printf("before serialize(): mem_usage %4.1f GB\n",  _to_gb(spp::GetProcessMemoryUsed()));
    // serialize age hash_map to "ages.dmp" file
    FILE *out = fopen("ages.dmp", "wb");
    age.serialize(FileSerializer(), out);
    fclose(out);

    printf("before clear(): mem_usage %4.1f GB\n",  _to_gb(spp::GetProcessMemoryUsed()));
    age.clear();
    printf("after clear(): mem_usage %4.1f GB\n",  _to_gb(spp::GetProcessMemoryUsed()));


    // read from "ages.dmp" file into age_read hash_map
    FILE *input = fopen("ages.dmp", "rb");
    age.unserialize(FileSerializer(), input);
    fclose(input);
    printf("after unserialize(): mem_usage %4.1f GB\n",  _to_gb(spp::GetProcessMemoryUsed()));
}

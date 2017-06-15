#include <iostream>
#include <algorithm>
#include <fstream>

#include <sparsepp/spp.h>
using spp::sparse_hash_map;

using namespace std;

struct StringToIntSerializer
{
    bool operator()(std::ofstream* stream, const std::pair<const std::string, int>& value) const
    {
        size_t sizeSecond = sizeof(value.second);
        size_t sizeFirst = value.first.size();
        stream->write((char*)&sizeFirst, sizeof(sizeFirst));
        stream->write(value.first.c_str(), sizeFirst);
        stream->write((char*)&value.second, sizeSecond);
        return true;
    }

    bool operator()(std::ifstream* istream, std::pair<const std::string, int>* value) const
    {
        // Read key
        size_t size = 0;
        istream->read((char*)&size, sizeof(size));
        char * first = new char[size];
        istream->read(first, size);
        new (const_cast<string *>(&value->first)) string(first, size);

        // Read value
        istream->read((char *)&value->second, sizeof(value->second));
        return true;
    }
};

int main(int , char* [])
{
    sparse_hash_map<string, int> users;

    users["John"] = 12345;
    users["Bob"] = 553;
    users["Alice"] = 82200;

    // Write users to file "data.dat"
    // ------------------------------
    std::ofstream* stream = new std::ofstream("data.dat", 
                                              std::ios::out | std::ios::trunc | std::ios::binary);
    users.serialize(StringToIntSerializer(), stream);
    stream->close();
    delete stream;

    // Read from file "data.dat" into users2
    // -------------------------------------
    sparse_hash_map<string, int> users2;
    std::ifstream* istream = new std::ifstream("data.dat");
    users2.unserialize(StringToIntSerializer(), istream);
    istream->close();
    delete istream;
    
    for (sparse_hash_map<string, int>::iterator it = users2.begin(); it != users2.end(); ++it)
        printf("users2: %s -> %d\n", it->first.c_str(), it->second);

}

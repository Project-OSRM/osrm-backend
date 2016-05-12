#ifndef FINGERPRINT_H
#define FINGERPRINT_H

#include <boost/uuid/uuid.hpp>
#include <type_traits>

namespace osrm
{
namespace util
{

// implements a singleton, i.e. there is one and only one conviguration object
struct FingerPrint
{
    static FingerPrint GetValid();
    const boost::uuids::uuid &GetFingerPrint() const;
    bool IsMagicNumberOK(const FingerPrint &other) const;
    bool TestGraphUtil(const FingerPrint &other) const;
    bool TestContractor(const FingerPrint &other) const;
    bool TestRTree(const FingerPrint &other) const;
    bool TestQueryObjects(const FingerPrint &other) const;

    unsigned magic_number;
    char md5_prepare[33];
    char md5_tree[33];
    char md5_graph[33];
    char md5_objects[33];

    // initialize to {6ba7b810-9dad-11d1-80b4-00c04fd430c8}
    boost::uuids::uuid named_uuid;
};

static_assert(sizeof(FingerPrint) == 152, "FingerPrint has unexpected size");
static_assert(std::is_trivial<FingerPrint>::value, "FingerPrint needs to be trivial.");
static_assert(std::is_pod<FingerPrint>::value, "FingerPrint needs to be a POD.");
}
}

#endif /* FingerPrint_H */

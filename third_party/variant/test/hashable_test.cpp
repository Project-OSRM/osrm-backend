#include <cstddef>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_set>
#include <utility>

#include <mapbox/variant.hpp>

using namespace mapbox::util;

void test_singleton()
{
    using V = variant<int>;

    V singleton = 5;

    if (std::hash<V>{}(singleton) != std::hash<int>{}(5))
    {
        std::cerr << "Expected variant hash to be the same as hash of its value\n";
        std::exit(EXIT_FAILURE);
    }
}

void test_default_hashable()
{
    using V = variant<int, double, std::string>;

    V var;

    // Check int hashes
    var = 1;

    if (std::hash<V>{}(var) != std::hash<int>{}(1))
    {
        std::cerr << "Expected variant hash to be the same as hash of its value\n";
        std::exit(EXIT_FAILURE);
    }

    // Check double hashes
    var = 23.4;

    if (std::hash<V>{}(var) != std::hash<double>{}(23.4))
    {
        std::cerr << "Expected variant hash to be the same as hash of its value\n";
        std::exit(EXIT_FAILURE);
    }

    // Check string hashes
    var = std::string{"Hello, World!"};

    if (std::hash<V>{}(var) != std::hash<std::string>{}("Hello, World!"))
    {
        std::cerr << "Expected variant hash to be the same as hash of its value\n";
        std::exit(EXIT_FAILURE);
    }
}

struct Hashable
{
    static const constexpr auto const_hash = 5;
};

namespace std {
template <>
struct hash<Hashable>
{
    std::size_t operator()(const Hashable&) const noexcept
    {
        return Hashable::const_hash;
    }
};
}

void test_custom_hasher()
{
    using V = variant<int, Hashable, double>;

    V var;

    var = Hashable{};

    if (std::hash<V>{}(var) != Hashable::const_hash)
    {
        std::cerr << "Expected variant hash to be the same as hash of its value\n";
        std::exit(EXIT_FAILURE);
    }
}

void test_hashable_in_container()
{
    using V = variant<int, std::string, double>;

    // won't compile if V is not Hashable
    std::unordered_set<V> vs;

    vs.insert(1);
    vs.insert(2.3);
    vs.insert("4");
}

struct Empty
{
};

struct Node;

using Tree = variant<Empty, recursive_wrapper<Node>>;

struct Node
{
    Node(Tree left_, Tree right_) : left(std::move(left_)), right(std::move(right_)) {}

    Tree left = Empty{};
    Tree right = Empty{};
};

namespace std {
template <>
struct hash<Empty>
{
    std::size_t operator()(const Empty&) const noexcept
    {
        return 3;
    }
};

template <>
struct hash<Node>
{
    std::size_t operator()(const Node& n) const noexcept
    {
        return 5 + std::hash<Tree>{}(n.left) + std::hash<Tree>{}(n.right);
    }
};
}

void test_recursive_hashable()
{

    Tree tree = Node{Node{Empty{}, Empty{}}, Empty{}};

    if (std::hash<Tree>{}(tree) != ((5 + (5 + (3 + 3))) + 3))
    {
        std::cerr << "Expected variant hash to be the same as hash of its value\n";
        std::exit(EXIT_FAILURE);
    }
}

int main()
{
    test_singleton();
    test_default_hashable();
    test_custom_hasher();
    test_hashable_in_container();
    test_recursive_hashable();
}

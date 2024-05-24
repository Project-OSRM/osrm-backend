// @EXPECTED: no matching function for call to .*\<apply_visitor\>

#include <mapbox/variant.hpp>

struct mutating_visitor
{
    mutating_visitor(int val)
        : val_(val) {}

    void operator()(int& val) const
    {
        val = val_;
    }

    int val_;
};

int main()
{
    const mapbox::util::variant<int> var(123);
    const mutating_visitor visitor(456);
    mapbox::util::apply_visitor(visitor, var);
}

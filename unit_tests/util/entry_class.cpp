#include "util/guidance/entry_class.hpp"

#include <boost/test/unit_test.hpp>

#include <climits>
#include <cstdint>
#include <limits>

BOOST_AUTO_TEST_SUITE(entry_class)

using osrm::util::guidance::EntryClass;

// How many roads the class can hold, which is how wide its flags are.  Taken from the
// class so that widening it cannot leave the test asserting the old capacity.
constexpr std::uint32_t CAPACITY = EntryClass::CAPACITY;
static_assert(CAPACITY == 64, "EntryClass capacity changed, and so did the .osrm.icd layout");

BOOST_AUTO_TEST_CASE(entry_class_records_what_it_is_given)
{
    EntryClass entries;
    BOOST_CHECK(entries.activate(0));
    BOOST_CHECK(entries.activate(CAPACITY - 1));

    BOOST_CHECK(entries.allowsEntry(0));
    BOOST_CHECK(entries.allowsEntry(CAPACITY - 1));
    BOOST_CHECK(!entries.allowsEntry(1));
}

// Asking about a road the class cannot hold answers, rather than aborting or shifting off
// the end of the type.
//
// The engine walks the bearings of an intersection and asks about each one, and there can
// be more bearings than there are bits here.  An ordinary junction never comes close, but
// a meshed pedestrian area does: every line of sight from a plaza vertex becomes a way,
// and a single vertex has been seen with 151 of them in Ile-de-France.
//
// Before this the shift was by the width of the type, which is undefined behaviour: with
// asserts on it aborted osrm-routed on any route reported with steps, and with them off it
// returned whatever the shift produced.
BOOST_AUTO_TEST_CASE(entry_class_answers_for_a_road_it_could_not_record)
{
    EntryClass entries;
    for (std::uint32_t i = 0; i < CAPACITY; ++i)
    {
        BOOST_REQUIRE(entries.activate(i));
    }

    // Storing one beyond capacity already reports that it did not happen.
    BOOST_CHECK(!entries.activate(CAPACITY));
    BOOST_CHECK(!entries.activate(151));

    // So reading it back says the same thing, for every index the engine might reach.
    BOOST_CHECK(!entries.allowsEntry(CAPACITY));
    BOOST_CHECK(!entries.allowsEntry(151));
    BOOST_CHECK(!entries.allowsEntry(CHAR_BIT * sizeof(std::uint64_t)));
    BOOST_CHECK(!entries.allowsEntry(std::numeric_limits<std::uint32_t>::max()));

    // And the ones it could record are untouched.
    for (std::uint32_t i = 0; i < CAPACITY; ++i)
    {
        BOOST_CHECK_MESSAGE(entries.allowsEntry(i), "road " << i << " should still allow entry");
    }
}

BOOST_AUTO_TEST_SUITE_END()

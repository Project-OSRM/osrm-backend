#ifndef UNIT_TESTS_RANDOM_SEED_HPP
#define UNIT_TESTS_RANDOM_SEED_HPP

#include <cstdlib>

namespace osrm::test
{

// Returns the fixed random seed for reproducible tests.
// The seed can be overridden via the OSRM_TEST_SEED environment variable.
// Default value is 0xdeadbeef.
inline unsigned getTestRandomSeed()
{
    const char *env_seed = std::getenv("OSRM_TEST_SEED");
    if (env_seed != nullptr && env_seed[0] != '\0')
    {
        char *end = nullptr;
        const auto value = std::strtoul(env_seed, &end, 0);
        if (end != env_seed && *end == '\0')
        {
            return static_cast<unsigned>(value);
        }
    }
    return 0xdeadbeef;
}

} // namespace osrm::test

#endif // UNIT_TESTS_RANDOM_SEED_HPP

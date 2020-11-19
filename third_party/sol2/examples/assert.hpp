#ifndef EXAMPLES_ASSERT_HPP
#define EXAMPLES_ASSERT_HPP

#ifdef SOL2_CI
struct pre_main {
	pre_main() {
		#ifdef _MSC_VER
		_set_abort_behavior(0, _WRITE_ABORT_MSG);
		#endif
	}
} pm;
#endif // Prevent lockup when doing Continuous Integration

#ifndef NDEBUG
#include <exception>
#include <iostream>
#include <cstdlib>

#   define m_assert(condition, message) \
    do { \
        if (! (condition)) { \
            std::cerr << "Assertion `" #condition "` failed in " << __FILE__ \
                      << " line " << __LINE__ << ": " << message << std::endl; \
            std::terminate(); \
        } \
    } while (false)

#   define c_assert(condition) \
    do { \
        if (! (condition)) { \
            std::cerr << "Assertion `" #condition "` failed in " << __FILE__ \
                      << " line " << __LINE__ << std::endl; \
            std::terminate(); \
        } \
    } while (false)
#else
#   define m_assert(condition, message) do { if (false) { (void)(condition); (void)sizeof(message); } } while (false)
#   define c_assert(condition) do { if (false) { (void)(condition); } } while (false)
#endif

#endif // EXAMPLES_ASSERT_HPP

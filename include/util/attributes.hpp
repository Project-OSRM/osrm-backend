#ifndef OSRM_ATTRIBUTES_HPP_
#define OSRM_ATTRIBUTES_HPP_

// OSRM_ATTR_WARN_UNUSED - caller has to use function's return value
// https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html

#if defined(__GNUC__) && (__GNUC__ >= 4)
#define OSRM_ATTR_WARN_UNUSED __attribute__((warn_unused_result))
#else
#define OSRM_ATTR_WARN_UNUSED
#endif

#endif

#if !defined(spp_stdint_h_guard)
#define spp_stdint_h_guard

#include <sparsepp/spp_config.h>

#if defined(SPP_HAS_CSTDINT) && (__cplusplus >= 201103)
    #include <cstdint>
#else
    #if defined(__FreeBSD__) || defined(__IBMCPP__) || defined(_AIX)
        #include <inttypes.h>
    #else
        #include <stdint.h>
    #endif
#endif

#endif // spp_stdint_h_guard

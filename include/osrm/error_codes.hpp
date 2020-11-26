#ifndef OSRM_ERRORCODES_HPP
#define OSRM_ERRORCODES_HPP

#include <string>

namespace osrm
{

/**
 * Various error codes that can be returned by OSRM internal functions.
 * Note: often, these translate into return codes from `int main()` functions.
 *       Thus, do not change the order - if adding new codes, append them to the
 *       end, so the code values do not change for users that are checking for
 *       certain values.
 */
enum ErrorCode
{
    InvalidFingerprint = 2, // Start at 2 to avoid colliding with POSIX EXIT_FAILURE
    IncompatibleFileVersion,
    FileOpenError,
    FileReadError,
    FileWriteError,
    FileIOError,
    UnexpectedEndOfFile,
    IncompatibleDataset,
    UnknownAlgorithm
#ifndef NDEBUG
    // Leave this at the end.  In debug mode, we assert that the size of
    // this enum matches the number of messages we have documented, and __ENDMARKER__
    // is used as the "count" value.
    ,
    __ENDMARKER__
#endif
};
} // namespace osrm

#endif // OSRM_ERRORCODES_HPP
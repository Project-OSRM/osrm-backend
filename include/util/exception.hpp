/*

Copyright (c) 2017, Project OSRM contributors
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef OSRM_EXCEPTION_HPP
#define OSRM_EXCEPTION_HPP

#include <array>
#include <exception>
#include <iostream>
#include <string>
#include <utility>

#include "osrm/error_codes.hpp"
#include <boost/format.hpp>

namespace osrm
{
namespace util
{

class exception : public std::exception
{
  public:
    explicit exception(const char *message) : message(message) {}
    explicit exception(std::string message) : message(std::move(message)) {}
    explicit exception(boost::format message) : message(message.str()) {}
    const char *what() const noexcept override { return message.c_str(); }

  private:
    // This function exists to 'anchor' the class, and stop the compiler from
    // copying vtable and RTTI info into every object file that includes
    // this header. (Caught by -Wweak-vtables under Clang.)
    virtual void anchor() const;
    const std::string message;
};

/**
 * Indicates a class of error that occurred that was caused by some kind of
 * external input (common examples are out of disk space, file permission errors,
 * user supplied bad data, etc).
 */

constexpr const std::array<const char *, 11> ErrorDescriptions = {{
    "",                                                      // Dummy - ErrorCode values start at 2
    "",                                                      // Dummy - ErrorCode values start at 2
    "Fingerprint did not match the expected value",          // InvalidFingerprint
    "File is incompatible with this version of OSRM",        // IncompatibleFileVersion
    "Problem opening file",                                  // FileOpenError
    "Problem reading from file",                             // FileReadError
    "Problem writing to file",                               // FileWriteError
    "I/O error occurred",                                    // FileIOError
    "Unexpected end of file",                                // UnexpectedEndOfFile
    "The dataset you are trying to load is not "             // IncompatibleDataset
    "compatible with the routing algorithm you want to use." // ...continued...
    "Incompatible algorithm"                                 // IncompatibleAlgorithm
}};

#ifndef NDEBUG
// Check that we have messages for every enum
static_assert(ErrorDescriptions.size() == ErrorCode::__ENDMARKER__,
              "ErrorCode list and ErrorDescription lists are different sizes");
#endif

class RuntimeError : public exception
{
    using Base = exception;
    using Base::Base;

  public:
    explicit RuntimeError(const std::string &message,
                          const ErrorCode code_,
                          const std::string &sourceref,
                          const char *possiblecause = nullptr)
        : Base(BuildMessage(message, code_, sourceref, possiblecause)), code(code_)
    {
    }

    ErrorCode GetCode() const { return code; }

  private:
    // This function exists to 'anchor' the class, and stop the compiler from
    // copying vtable and RTTI info into every object file that includes
    // this header. (Caught by -Wweak-vtables under Clang.)
    virtual void anchor() const;
    const ErrorCode code;

    static std::string BuildMessage(const std::string &message,
                                    const ErrorCode code_,
                                    const std::string &sourceref,
                                    const char *possiblecause = nullptr)
    {
        std::string result;
        result += ErrorDescriptions[code_];
        result += ": " + std::string(message);
        result += possiblecause != nullptr
                      ? (std::string(" (possible cause: \"") + possiblecause + "\")")
                      : "";
        result += " (at " + sourceref + ")";
        return result;
    }
};
} // namespace util
} // namespace osrm

#endif /* OSRM_EXCEPTION_HPP */

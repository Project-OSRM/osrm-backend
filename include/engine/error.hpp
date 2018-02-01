#ifndef OSRM_ENGINE_ERROR_HPP
#define OSRM_ENGINE_ERROR_HPP

#include "util/exception.hpp"

#include <string>
#include <array>

namespace osrm
{
namespace engine
{

enum class ErrorCode
{
    NO_ERROR = 0,
    NOT_IMPLEMENTED,
    INVALID_VALUE,
    INVALID_OPTIONS,
    TOO_BIG,
    NO_ROUTE,
    NO_SEGMENT,
    NO_TABLE,
    NO_TRIP,
    NO_MATCH,
    NUM_ERROR,
};

inline std::string codeToString(ErrorCode code)
{
    static std::array<const char *, static_cast<std::size_t>(ErrorCode::NUM_ERROR)> names{
        {"Ok",
         "NotImplemented",
         "InvalidValue",
         "InvalidOptions",
         "TooBig",
         "NoRoute",
         "NoSegment",
         "NoTable",
         "NoTrip",
         "NoMatch"}};

    return names[static_cast<std::size_t>(code)];
}

struct Error
{
    ErrorCode code = ErrorCode::NO_ERROR;
    std::string message;

    static auto NO_ERROR() { return Error{}; }

    auto throwException() const
    {
        throw util::exception(std::string(codeToString(code)) + ":" + message);
    }
};

template<typename ResultT>
struct MaybeResult
{
    MaybeResult(Error error_)
        : error(std::move(error_))
    {
    }

    MaybeResult(ResultT result_)
        : result(std::move(result_))
    {
        BOOST_ASSERT(error.code == ErrorCode::NO_ERROR);
    }

    operator bool() const {
        return error.code == ErrorCode::NO_ERROR;
    }

    operator const Error &() const {
        return error;
    }

    operator const ResultT &() const {
        if (error.code != ErrorCode::NO_ERROR)
        {
            error.throwException();
        }

        return result;
    }

private:
    Error error;
    ResultT result;
};

}
}

#endif

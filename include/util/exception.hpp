#ifndef EXCEPTION_HPP
#define EXCEPTION_HPP

#include <exception>
#include <string>
#include <utility>

namespace osrm
{
namespace util
{

class exception final : public std::exception
{
  public:
    explicit exception(const char *message) : message(message) {}
    explicit exception(std::string message) : message(std::move(message)) {}

  private:
    // This function exists to 'anchor' the class, and stop the compiler from
    // copying vtable and RTTI info into every object file that includes
    // this header. (Caught by -Wweak-vtables under Clang.)
    virtual void anchor() const;
    const char *what() const noexcept override { return message.c_str(); }
    const std::string message;
};
}
}

#endif /* EXCEPTION_HPP */

#ifndef OSRM_UTIL_FUNCTION_REF_HPP
#define OSRM_UTIL_FUNCTION_REF_HPP

#include <memory>
#include <type_traits>
#include <utility>

namespace osrm::util
{

/**
 * Non-owning view of a callable, for parameters that only need the callable for
 * the duration of the call.
 *
 * Unlike std::function this stores a plain object pointer next to a plain
 * function pointer. There is no small-buffer union and no allocation, so
 * passing a lambda through a virtual interface costs two pointers and cannot
 * throw. The referenced callable has to outlive the FunctionRef.
 */
template <typename Signature> class FunctionRef;

template <typename Result, typename... Args> class FunctionRef<Result(Args...)>
{
  public:
    template <typename Callable,
              typename = std::enable_if_t<!std::is_same_v<std::decay_t<Callable>, FunctionRef> &&
                                          std::is_invocable_r_v<Result, Callable &, Args...>>>
    FunctionRef(Callable &&callable) noexcept
        : erased(const_cast<void *>(static_cast<const void *>(std::addressof(callable)))),
          invoker{[](void *object, Args... args) -> Result
                  {
                      return (*static_cast<std::remove_reference_t<Callable> *>(object))(
                          std::forward<Args>(args)...);
                  }}
    {
    }

    Result operator()(Args... args) const { return invoker(erased, std::forward<Args>(args)...); }

  private:
    void *erased;
    Result (*invoker)(void *, Args...);
};

} // namespace osrm::util

#endif // OSRM_UTIL_FUNCTION_REF_HPP

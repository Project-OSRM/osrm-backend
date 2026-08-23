#ifndef OSRM_UTIL_FUNCTION_REF_HPP
#define OSRM_UTIL_FUNCTION_REF_HPP

#include <memory>
#include <type_traits>
#include <utility>

namespace osrm::util
{

/**
 * Non-owning view of a callable, for parameters that only need it for the
 * duration of the call.
 *
 * Unlike std::function this is an object pointer next to a function pointer.
 * There is no small-buffer union and no allocation, so passing a lambda through
 * a virtual interface costs two pointers and cannot throw. The absence of the
 * union also keeps GCC from reporting a maybe-uninitialized read of the buffer
 * on paths it inlined, which is what std::function does under -flto.
 *
 * This refers to the callable rather than owning it, so the callable has to
 * outlive the FunctionRef. That holds for a parameter, where the argument lives
 * until the end of the full expression, and it does not hold for a member or a
 * return value. Do not store one.
 *
 * The callable has to be an object, which covers lambdas, function objects and
 * function pointers. A plain function is not an object and there is nothing to
 * point at, so wrap it in a lambda or take its address into a variable first.
 */
template <typename Signature> class FunctionRef;

template <typename Result, typename... Args> class FunctionRef<Result(Args...)>
{
  public:
    template <typename Callable,
              typename Object = std::remove_reference_t<Callable>,
              typename = std::enable_if_t<!std::is_same_v<std::decay_t<Callable>, FunctionRef> &&
                                          std::is_object_v<Object> &&
                                          std::is_invocable_r_v<Result, Callable &, Args...>>>
    FunctionRef(Callable &&callable) noexcept
        : erased(const_cast<void *>(static_cast<const void *>(std::addressof(callable)))),
          invoker{[](void *object, Args... args) -> Result
                  { return (*static_cast<Object *>(object))(std::forward<Args>(args)...); }}
    {
    }

    Result operator()(Args... args) const { return invoker(erased, std::forward<Args>(args)...); }

  private:
    void *erased;
    Result (*invoker)(void *, Args...);
};

} // namespace osrm::util

#endif // OSRM_UTIL_FUNCTION_REF_HPP

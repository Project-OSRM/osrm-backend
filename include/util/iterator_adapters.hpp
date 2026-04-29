#ifndef OSRM_UTIL_ITERATOR_ADAPTERS_HPP
#define OSRM_UTIL_ITERATOR_ADAPTERS_HPP

#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace osrm::util
{
// function_output_iterator: calls a callable when assigned to
template <typename Func> class function_output_iterator
{
  private:
    struct proxy
    {
        Func *func;
        template <typename T> proxy &operator=(T &&value)
        {
            (*func)(std::forward<T>(value));
            return *this;
        }
    };

  public:
    using iterator_category = std::output_iterator_tag;
    using value_type = void;
    using difference_type = void;
    using pointer = void;
    using reference = void;

    explicit function_output_iterator(Func f) : func(std::move(f)) {}

    proxy operator*() { return proxy{&func}; }

    function_output_iterator &operator++() { return *this; }
    function_output_iterator operator++(int) { return *this; }

  private:
    Func func;
};

template <typename Func> inline auto make_function_output_iterator(Func f)
{
    return function_output_iterator<Func>(std::move(f));
}

// function_input_iterator: returns value from callable on deref, ++ is a no-op
template <typename Func> class function_input_iterator
{
  public:
    using iterator_category = std::input_iterator_tag;
    using value_type = std::remove_cv_t<std::remove_reference_t<decltype(std::declval<Func>()())>>;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type *;
    using reference = value_type;

    explicit function_input_iterator(Func f) : func(std::move(f)) {}

    value_type operator*() const { return func(); }
    function_input_iterator &operator++() { return *this; }
    function_input_iterator operator++(int) { return *this; }

  private:
    Func func;
};

template <typename Func> inline auto make_function_input_iterator(Func f)
{
    return function_input_iterator<Func>(std::move(f));
}

// permutation_iterator: iterates over underlying container in order specified by index iterator
template <typename BaseIt, typename IndexIt> class permutation_iterator
{
  public:
    using index_traits = std::iterator_traits<IndexIt>;
    using base_traits = std::iterator_traits<BaseIt>;
    using iterator_category = typename index_traits::iterator_category;
    using value_type = typename base_traits::value_type;
    using difference_type = typename index_traits::difference_type;
    using pointer = typename base_traits::pointer;
    using reference = typename base_traits::reference;

    permutation_iterator() = default;
    permutation_iterator(BaseIt base, IndexIt index_it) : base(base), index_it(index_it) {}

    reference operator*() const { return *(base + static_cast<std::size_t>(*index_it)); }

    permutation_iterator &operator++()
    {
        ++index_it;
        return *this;
    }
    permutation_iterator operator++(int)
    {
        auto tmp = *this;
        ++*this;
        return tmp;
    }

    permutation_iterator &operator--()
    {
        --index_it;
        return *this;
    }
    permutation_iterator operator--(int)
    {
        auto tmp = *this;
        --*this;
        return tmp;
    }

    permutation_iterator &operator+=(difference_type n)
    {
        index_it += n;
        return *this;
    }
    permutation_iterator &operator-=(difference_type n)
    {
        index_it -= n;
        return *this;
    }

    friend permutation_iterator operator+(permutation_iterator it, difference_type n)
    {
        it += n;
        return it;
    }
    friend permutation_iterator operator-(permutation_iterator it, difference_type n)
    {
        it -= n;
        return it;
    }

    friend difference_type operator-(const permutation_iterator &a, const permutation_iterator &b)
    {
        return a.index_it - b.index_it;
    }

    reference operator[](difference_type n) const
    {
        return *(base + static_cast<std::size_t>(*(index_it + n)));
    }

    friend bool operator==(const permutation_iterator &a, const permutation_iterator &b)
    {
        return a.index_it == b.index_it;
    }
    friend bool operator!=(const permutation_iterator &a, const permutation_iterator &b)
    {
        return !(a == b);
    }

  private:
    BaseIt base{};
    IndexIt index_it{};
};

template <typename BaseIt, typename IndexIt>
inline auto make_permutation_iterator(BaseIt base, IndexIt index_it)
{
    return permutation_iterator<BaseIt, IndexIt>(base, index_it);
}

} // namespace osrm::util

#endif // OSRM_UTIL_ITERATOR_ADAPTERS_HPP

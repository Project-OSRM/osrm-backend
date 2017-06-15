#if !defined(spp_traits_h_guard)
#define spp_traits_h_guard

#include <sparsepp/spp_config.h>

template<int S, int H> class HashObject; // for Google's benchmark, not in spp namespace!

namespace spp_
{

// ---------------------------------------------------------------------------
//                       type_traits we need
// ---------------------------------------------------------------------------
template<class T, T v>
struct integral_constant { static const T value = v; };

template <class T, T v> const T integral_constant<T, v>::value;

typedef integral_constant<bool, true>  true_type;
typedef integral_constant<bool, false> false_type;

typedef integral_constant<int, 0>      zero_type;
typedef integral_constant<int, 1>      one_type;
typedef integral_constant<int, 2>      two_type;
typedef integral_constant<int, 3>      three_type;

template<typename T, typename U> struct is_same : public false_type { };
template<typename T> struct is_same<T, T> : public true_type { };

template<typename T> struct remove_const { typedef T type; };
template<typename T> struct remove_const<T const> { typedef T type; };

template<typename T> struct remove_volatile { typedef T type; };
template<typename T> struct remove_volatile<T volatile> { typedef T type; };

template<typename T> struct remove_cv 
{
    typedef typename remove_const<typename remove_volatile<T>::type>::type type;
};

// ---------------- is_integral ----------------------------------------
template <class T> struct is_integral;
template <class T> struct is_integral         : false_type { };
template<> struct is_integral<bool>           : true_type { };
template<> struct is_integral<char>           : true_type { };
template<> struct is_integral<unsigned char>  : true_type { };
template<> struct is_integral<signed char>    : true_type { };
template<> struct is_integral<short>          : true_type { };
template<> struct is_integral<unsigned short> : true_type { };
template<> struct is_integral<int>            : true_type { };
template<> struct is_integral<unsigned int>   : true_type { };
template<> struct is_integral<long>           : true_type { };
template<> struct is_integral<unsigned long>  : true_type { };
#ifdef SPP_HAS_LONG_LONG
    template<> struct is_integral<long long>  : true_type { };
    template<> struct is_integral<unsigned long long> : true_type { };
#endif
template <class T> struct is_integral<const T>          : is_integral<T> { };
template <class T> struct is_integral<volatile T>       : is_integral<T> { };
template <class T> struct is_integral<const volatile T> : is_integral<T> { };

// ---------------- is_floating_point ----------------------------------------
template <class T> struct is_floating_point;
template <class T> struct is_floating_point      : false_type { };
template<> struct is_floating_point<float>       : true_type { };
template<> struct is_floating_point<double>      : true_type { };
template<> struct is_floating_point<long double> : true_type { };
template <class T> struct is_floating_point<const T> :        is_floating_point<T> { };
template <class T> struct is_floating_point<volatile T>       : is_floating_point<T> { };
template <class T> struct is_floating_point<const volatile T> : is_floating_point<T> { };

//  ---------------- is_pointer ----------------------------------------
template <class T> struct is_pointer;
template <class T> struct is_pointer     : false_type { };
template <class T> struct is_pointer<T*> : true_type { };
template <class T> struct is_pointer<const T>          : is_pointer<T> { };
template <class T> struct is_pointer<volatile T>       : is_pointer<T> { };
template <class T> struct is_pointer<const volatile T> : is_pointer<T> { };

//  ---------------- is_reference ----------------------------------------
template <class T> struct is_reference;
template<typename T> struct is_reference     : false_type {};
template<typename T> struct is_reference<T&> : true_type {};

//  ---------------- is_relocatable ----------------------------------------
// relocatable values can be moved around in memory using memcpy and remain
// correct. Most types are relocatable, an example of a type who is not would
// be a struct which contains a pointer to a buffer inside itself - this is the
// case for std::string in gcc 5.
// ------------------------------------------------------------------------
template <class T> struct is_relocatable;
template <class T> struct is_relocatable :
     integral_constant<bool, (is_integral<T>::value || is_floating_point<T>::value)>
{ };

template<int S, int H> struct is_relocatable<HashObject<S, H> > : true_type { };

template <class T> struct is_relocatable<const T>          : is_relocatable<T> { };
template <class T> struct is_relocatable<volatile T>       : is_relocatable<T> { };
template <class T> struct is_relocatable<const volatile T> : is_relocatable<T> { };
template <class A, int N> struct is_relocatable<A[N]>      : is_relocatable<A> { };
template <class T, class U> struct is_relocatable<std::pair<T, U> > :
     integral_constant<bool, (is_relocatable<T>::value && is_relocatable<U>::value)>
{ };

// A template helper used to select A or B based on a condition.
// ------------------------------------------------------------
template<bool cond, typename A, typename B>
struct if_
{
    typedef A type;
};

template<typename A, typename B>
struct if_<false, A, B> 
{
    typedef B type;
};

}  // spp_ namespace

#endif // spp_traits_h_guard

// Copyright Antony Polukhin, 2016.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_STACKTRACE_CONST_ITERATOR_HPP
#define BOOST_STACKTRACE_CONST_ITERATOR_HPP

#include <boost/config.hpp>
#ifdef BOOST_HAS_PRAGMA_ONCE
#   pragma once
#endif

#include <boost/iterator/iterator_facade.hpp>
#include <boost/assert.hpp>

#include <boost/stacktrace/frame.hpp>

namespace boost { namespace stacktrace {


#ifdef BOOST_STACKTRACE_DOXYGEN_INVOKED
/// Random access iterator over frames that returns boost::stacktrace::frame on dereference
class const_iterator: implementation_details {};

#else

// Forward declarations
template <std::size_t> class basic_stacktrace;

class const_iterator: public boost::iterator_facade<
    const_iterator,
    frame,
    boost::random_access_traversal_tag,
    frame>
{
    typedef boost::iterator_facade<
        const_iterator,
        frame,
        boost::random_access_traversal_tag,
        frame
    > base_t;

    const boost::stacktrace::detail::backend* impl_;
    std::size_t frame_no_;

    const_iterator(const boost::stacktrace::detail::backend* impl, std::size_t frame_no) BOOST_NOEXCEPT
        : impl_(impl)
        , frame_no_(frame_no)
    {}

    template <std::size_t> friend class basic_stacktrace;
    friend class ::boost::iterators::iterator_core_access;

    frame dereference() const BOOST_NOEXCEPT {
        return frame(impl_->get_address(frame_no_));
    }

    bool equal(const const_iterator& it) const BOOST_NOEXCEPT {
        return impl_ == it.impl_ && frame_no_ == it.frame_no_;
    }

    void increment() BOOST_NOEXCEPT {
        ++frame_no_;
    }

    void decrement() BOOST_NOEXCEPT {
        --frame_no_;
    }

    void advance(std::size_t n) BOOST_NOEXCEPT {
        frame_no_ += n;
    }

    base_t::difference_type distance_to(const const_iterator& it) const {
        BOOST_ASSERT(impl_ == it.impl_);
        return it.frame_no_ - frame_no_;
    }
};

#endif // #ifdef BOOST_STACKTRACE_DOXYGEN_INVOKED

}} // namespace boost::stacktrace

#endif // BOOST_STACKTRACE_CONST_ITERATOR_HPP

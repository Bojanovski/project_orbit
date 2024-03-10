  // -*- C++ -*-
  //===----------------------------------------------------------------------===//
  // Modifications Copyright Intel Corporation 2022
  // SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
  //===----------------------------------------------------------------------===//
  // Auto-generated from boost/mp11 sources https://github.com/boostorg/mp11

#ifndef SYCL_DETAIL_BOOST_MP11_DETAIL_MP_RENAME_HPP_INCLUDED
#define SYCL_DETAIL_BOOST_MP11_DETAIL_MP_RENAME_HPP_INCLUDED

//  Copyright 2015-2021 Peter Dimov.
//
//  Distributed under the Boost Software License, Version 1.0.
//
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
namespace sycl
{
namespace detail
{
namespace boost
{
namespace mp11
{

// mp_rename<L, B>
namespace detail
{

template<class A, template<class...> class B> struct mp_rename_impl
{
// An error "no type named 'type'" here means that the first argument to mp_rename is not a list
};

template<template<class...> class A, class... T, template<class...> class B> struct mp_rename_impl<A<T...>, B>
{
    using type = B<T...>;
};

} // namespace detail

template<class A, template<class...> class B> using mp_rename = typename detail::mp_rename_impl<A, B>::type;

template<template<class...> class F, class L> using mp_apply = typename detail::mp_rename_impl<L, F>::type;

template<class Q, class L> using mp_apply_q = typename detail::mp_rename_impl<L, Q::template fn>::type;

} // namespace mp11
} // namespace boost
} // namespace detail
} // namespace sycl

#endif // #ifndef SYCL_DETAIL_BOOST_MP11_DETAIL_MP_RENAME_HPP_INCLUDED

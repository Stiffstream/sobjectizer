/*
 * SObjectizer-5
 */

/*!
 * \file
 * \since
 * v.5.5.4
 *
 * \brief Lambda-related type traits.
 */

#pragma once

#include <type_traits>

namespace so_5
{

namespace details
{

namespace lambda_traits
{

/*!
 * \brief Detector of plain type without const/volatile modifiers.
 */
template< typename M >
struct plain_argument_type
	{
		using type =
				typename std::remove_cv<
						typename std::remove_reference< M >::type >::type;
	};

/*!
 * \brief Detector of lambda result and argument type.
 */
template< typename L >
struct traits
	: 	public traits< decltype(&L::operator()) >
	{
		//! Type to be used to pass lambda as argument to another function.
		/*!
		 * \since
		 * v.5.5.22
		 */
		using pass_by_type = L;
	};

/*!
 * \brief Specialization of lambda traits for const-lambda.
 */
template< class L, class R, class M >
struct traits< R (L::*)(M) const >
	{
		//! Type to be used to pass lambda as argument to another function.
		/*!
		 * \since
		 * v.5.5.22
		 */
		using pass_by_type = R(L::*)(M) const;

		//! Type of lambda result value.
		using result_type = R;
		//! Type of lambda argument.
		using argument_type = typename plain_argument_type< M >::type;

		//! Helper for calling a lambda.
		static R call_with_arg( L l, M m )
			{
				return l(m);
			}
	};

/*!
 * \brief Specialization of lambda traits for mutable lambda.
 */
template< class L, class R, class M >
struct traits< R (L::*)(M) >
	{
		//! Type to be used to pass lambda as argument to another function.
		/*!
		 * \since
		 * v.5.5.22
		 */
		using pass_by_type = R(L::*)(M);

		//! Type of lambda result value.
		using result_type = R;
		//! Type of lambda argument.
		using argument_type = typename plain_argument_type< M >::type;

		//! Helper for calling a lambda.
		static R call_with_arg( L l, M m )
			{
				return l(m);
			}
	};

/*!
 * \brief Specialization of lambda traits for const-lambda without argument.
 */
template< class L, class R >
struct traits< R (L::*)() const >
	{
		//! Type to be used to pass lambda as argument to another function.
		/*!
		 * \since
		 * v.5.5.22
		 */
		using pass_by_type = R(L::*)() const;

		//! Type of lambda result value.
		using result_type = R;

		//! Helper for calling a lambda.
		static R call_without_arg( L l )
			{
				return l();
			}
	};

/*!
 * \brief Specialization of lambda traits for mutable lambda without argument.
 */
template< class L, class R >
struct traits< R (L::*)() >
	{
		//! Type to be used to pass lambda as argument to another function.
		/*!
		 * \since
		 * v.5.5.22
		 */
		using pass_by_type = R(L::*)();
		
		//! Type of lambda result value.
		using result_type = R;

		//! Helper for calling a lambda.
		static R call_without_arg( L l )
			{
				return l();
			}
	};

/*!
 * \brief Specialization of lambda traits for ordinary function pointer.
 */
template< class R, class M >
struct traits< R(*)(M) >
	{
		//! Type to be used to pass lambda as argument to another function.
		/*!
		 * \since
		 * v.5.5.22
		 */
		using pass_by_type = R(*)(M);

		//! Type of lambda result value.
		using result_type = R;
		//! Type of lambda argument.
		using argument_type = typename plain_argument_type< M >::type;

		//! Helper for calling a lambda.
		static R call_with_arg( R (*l)(M), M m )
			{
				return l(m);
			}
	};

/*!
 * \brief Specialization of lambda traits for reference to ordinary function.
 */
template< class R, class M >
struct traits< R(&)(M) >
	{
		//! Type to be used to pass lambda as argument to another function.
		/*!
		 * \since
		 * v.5.5.22
		 */
		using pass_by_type = R(*)(M);

		//! Type of lambda result value.
		using result_type = R;
		//! Type of lambda argument.
		using argument_type = typename plain_argument_type< M >::type;

		//! Helper for calling a lambda.
		static R call_with_arg( R (*l)(M), M m )
			{
				return l(m);
			}
	};

// Please note: there are no traits for R(*)() and R(&)().
// It is because argument-less handlers is going to be disabled in
// the next major version of SObjectizer.

namespace impl
{

/*!
 * \brief A checker for lambda likeness.
 */
template< typename L, typename = void >
struct has_func_call_operator : public std::false_type {};

template< typename L >
struct has_func_call_operator< L, std::void_t< decltype(&L::operator()) > >
	: public std::true_type
	{};

/*!
 * \brief A detector of lambda argument type if the checked type is lambda.
 */
template< bool is_lambda, class L >
struct argument_if_lambda
	{};

/*!
 * \brief A specialization of lambda argument detector for the case
 * when the checked type is a lambda.
 */
template< class L >
struct argument_if_lambda< true, L >
	{
		using type = typename traits< L >::argument_type;
	};

} /* namespace impl */

/*!
 * \brief A detector of lambda argument type if the checked type is lambda.
 */
template< class L >
using argument_type_if_lambda = impl::argument_if_lambda<
		impl::has_func_call_operator< L >::value, L >;

/*!
 * \brief A detector that type is a lambda or functional object.
 *
 * \since
 * v.5.5.20
 */
template<typename L>
struct is_lambda
	{
		static constexpr const bool value = impl::has_func_call_operator<L>::value;
	};

} /* namespace lambda_traits */

} /* namespace details */

} /* namespace so_5 */


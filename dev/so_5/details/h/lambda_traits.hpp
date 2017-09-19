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
	{};

/*!
 * \brief Specialization of lambda traits for const-lambda.
 */
template< class L, class R, class M >
struct traits< R (L::*)(M) const >
	{
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
		//! Type of lambda result value.
		using result_type = R;

		//! Helper for calling a lambda.
		static R call_without_arg( L l )
			{
				return l();
			}
	};

namespace impl
{

/*!
 * \brief A checker for lambda likeness.
 */
template< typename L >
class has_func_call_operator
	{
	private :
		template< typename P, P > struct checker;

		template< typename D > static std::true_type test(
				checker< decltype(&D::operator()), &D::operator()> * );

		template< typename D > static std::false_type test(...);

	public :
		enum { value =
			std::is_same< std::true_type, decltype(test<L>(nullptr)) >::value };
	};

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
struct argument_type_if_lambda
	:	public impl::argument_if_lambda<
			impl::has_func_call_operator< L >::value, L >
	{};

} /* namespace lambda_traits */

} /* namespace details */

} /* namespace so_5 */


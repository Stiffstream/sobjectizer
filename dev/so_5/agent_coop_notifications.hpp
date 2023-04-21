/*
	SObjectizer 5.
*/

/*!
	\since
	v.5.2.3 

	\file
	\brief Agent cooperation notifications definition.
*/

#pragma once

#include <so_5/coop.hpp>
#include <so_5/mbox.hpp>
#include <so_5/send_functions.hpp>

#if defined( SO_5_MSVC )
	#pragma warning(push)
	#pragma warning(disable: 4251)
#endif

namespace so_5
{

//
// msg_coop_registered
//
/*!
 * \since
 * v.5.2.3
 *
 * \brief Message about cooperation registration completion.
 */
struct SO_5_TYPE msg_coop_registered : public message_t
{
	// Note: clang-3.9 requires this on Windows platform.
	msg_coop_registered( const msg_coop_registered & ) = delete;
	msg_coop_registered( msg_coop_registered && ) = delete;
	msg_coop_registered & operator=( const msg_coop_registered & ) = delete;
	msg_coop_registered & operator=( msg_coop_registered && ) = delete;

	msg_coop_registered(
		coop_handle_t coop ) noexcept
		:	m_coop( std::move(coop) )
		{}

	const coop_handle_t m_coop;
};

//
// msg_coop_deregistered
//
/*!
 * \since
 * v.5.2.3
 *
 * \brief Message about cooperation deregistration completion.
 */
struct SO_5_TYPE msg_coop_deregistered : public message_t
{
	// Note: clang-3.9 requires this on Windows platform.
	msg_coop_deregistered( const msg_coop_deregistered & ) = delete;
	msg_coop_deregistered( msg_coop_deregistered && ) = delete;
	msg_coop_deregistered & operator=( const msg_coop_deregistered & ) = delete;
	msg_coop_deregistered & operator=( msg_coop_deregistered && ) = delete;

	msg_coop_deregistered(
		coop_handle_t coop,
		coop_dereg_reason_t reason ) noexcept
		:	m_coop( std::move(coop) )
		,	m_reason( reason )
		{}

	const coop_handle_t m_coop;
	const coop_dereg_reason_t m_reason;
};

//
// make_coop_reg_notificator
//
/*!
 * \since
 * v.5.2.3
 *
 * \brief Create notificator about cooperation registration completion.
 *
 * \attention
 * Since v.5.6.0 reg_notificators should be noexcept functions/functors.
 * But this notificator uses so_5::send() function inside. This function
 * can throw (for example std::bad_alloc can be raised if there is no
 * enough memory available). It means that if so_5::send() throws inside
 * this notificator the whole application will be aborted via
 * std::terminate().
 */
inline auto
make_coop_reg_notificator(
	//! A mbox to which msg_coop_registered will be sent.
	mbox_t target ) noexcept
{
	class actual_notificator_t final
		{
			mbox_t m_target;

		public :
			actual_notificator_t( mbox_t target ) noexcept
				:	m_target{ std::move(target) }
				{}

			void operator()(
				environment_t & /*env*/,
				const coop_handle_t & handle ) const noexcept
				{
					so_5::send< msg_coop_registered >( m_target, handle );
				}
		};

	return actual_notificator_t{ std::move(target) };
}

//
// make_coop_dereg_notificator
//
/*!
 * \since
 * v.5.2.3
 *
 * \brief Create notificator about cooperation deregistration completion.
 *
 * \attention
 * Since v.5.6.0 dereg_notificators should be noexcept functions/functors.
 * But this notificator uses so_5::send() function inside. This function
 * can throw (for example std::bad_alloc can be raised if there is no
 * enough memory available). It means that if so_5::send() throws inside
 * this notificator the whole application will be aborted via
 * std::terminate().
 */
inline auto
make_coop_dereg_notificator(
	//! A mbox to which msg_coop_deregistered will be sent.
	mbox_t target ) noexcept
{
	class actual_notificator_t final
		{
			mbox_t m_target;

		public :
			actual_notificator_t( mbox_t target ) noexcept
				:	m_target{ std::move(target) }
				{}

			void operator()(
				environment_t & /*env*/,
				const coop_handle_t & handle,
				const coop_dereg_reason_t & reason ) const noexcept
				{
					so_5::send< msg_coop_deregistered >( m_target, handle, reason );
				}
		};

	return actual_notificator_t{ std::move(target) };
}

} /* namespace so_5 */

#if defined( SO_5_MSVC )
	#pragma warning(pop)
#endif


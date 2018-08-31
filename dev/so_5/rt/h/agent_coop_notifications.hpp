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

#include <so_5/rt/h/agent_coop.hpp>
#include <so_5/rt/h/mbox.hpp>

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
		std::string coop_name )
		:	m_coop_name( std::move(coop_name) )
		{}

	const std::string m_coop_name;
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
		std::string coop_name,
		coop_dereg_reason_t reason )
		:	m_coop_name( std::move(coop_name) )
		,	m_reason( reason )
		{}

	const std::string m_coop_name;
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
 */
SO_5_FUNC coop_reg_notificator_t
make_coop_reg_notificator(
	//! A mbox to which msg_coop_registered will be sent.
	const mbox_t & mbox );

//
// make_coop_dereg_notificator
//
/*!
 * \since
 * v.5.2.3
 *
 * \brief Create notificator about cooperation deregistration completion.
 */
SO_5_FUNC coop_dereg_notificator_t
make_coop_dereg_notificator(
	//! A mbox to which msg_coop_deregistered will be sent.
	const mbox_t & mbox );

namespace rt
{

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::msg_coop_registered
 * instead.
 */
using msg_coop_registered = so_5::msg_coop_registered;

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::msg_coop_deregistered
 * instead.
 */
using msg_coop_deregistered = so_5::msg_coop_deregistered;

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::make_coop_reg_notificator
 * instead.
 */
inline so_5::coop_reg_notificator_t
make_coop_reg_notificator(
	//! A mbox to which msg_coop_registered will be sent.
	const mbox_t & mbox )
	{
		return so_5::make_coop_reg_notificator( mbox );
	}

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::make_coop_dereg_notificator
 * instead.
 */
inline coop_dereg_notificator_t
make_coop_dereg_notificator(
	//! A mbox to which msg_coop_deregistered will be sent.
	const mbox_t & mbox )
	{
		return so_5::make_coop_dereg_notificator( mbox );
	}

} /* namespace rt */

} /* namespace so_5 */

#if defined( SO_5_MSVC )
	#pragma warning(pop)
#endif


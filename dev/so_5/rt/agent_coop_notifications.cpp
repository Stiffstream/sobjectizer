/*
	SObjectizer 5.
*/

/*!
	\since v.5.2.3 
	\file
	\brief Agent cooperation notifications definition.
*/

#include <so_5/rt/h/agent_coop_notifications.hpp>

namespace so_5
{

//
// msg_coop_registered
//
msg_coop_registered::msg_coop_registered(
	const std::string & coop_name )
	:	m_coop_name( coop_name )
	{}

msg_coop_registered::~msg_coop_registered()
	{}

//
// msg_coop_deregistered
//
msg_coop_deregistered::msg_coop_deregistered(
	const std::string & coop_name,
	coop_dereg_reason_t reason )
	:	m_coop_name( coop_name )
	,	m_reason( std::move( reason ) )
	{}

msg_coop_deregistered::~msg_coop_deregistered()
	{}

//
// make_coop_reg_notificator
//
SO_5_FUNC coop_reg_notificator_t
make_coop_reg_notificator(
	const mbox_t & mbox )
	{
		return [mbox](
				environment_t &,
				const std::string & coop_name )
				{
					mbox->deliver_message( new msg_coop_registered( coop_name ) );
				};
	}

//
// make_coop_dereg_notificator
//
SO_5_FUNC coop_dereg_notificator_t
make_coop_dereg_notificator(
	const mbox_t & mbox )
	{
		return [mbox](
				environment_t &,
				const std::string & coop_name,
				const coop_dereg_reason_t & reason )
				{
					mbox->deliver_message(
							new msg_coop_deregistered(
									coop_name,
									reason ) );
				};
	}

} /* namespace so_5 */



/*
	SObjectizer 5.
*/

/*!
	\since
	v.5.2.3 

	\file
	\brief Agent cooperation notifications definition.
*/

#include <so_5/rt/h/agent_coop_notifications.hpp>

#include <so_5/h/stdcpp.hpp>

namespace so_5
{

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
					auto msg = stdcpp::make_unique<msg_coop_registered>( coop_name );
					mbox->deliver_message( std::move(msg) );
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
					auto msg = stdcpp::make_unique<msg_coop_deregistered>(
							coop_name,
							reason );
					mbox->deliver_message( std::move(msg) );
				};
	}

} /* namespace so_5 */



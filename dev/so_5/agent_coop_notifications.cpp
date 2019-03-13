/*
	SObjectizer 5.
*/

/*!
	\since
	v.5.2.3 

	\file
	\brief Agent cooperation notifications definition.
*/

#include <so_5/agent_coop_notifications.hpp>

#include <so_5/send_functions.hpp>

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
				const coop_handle_t & coop )
				{
					so_5::send< msg_coop_registered >( mbox, coop );
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
				const coop_handle_t & coop,
				const coop_dereg_reason_t & reason )
				{
					so_5::send< msg_coop_deregistered >(
							mbox,
							coop,
							reason );
				};
	}

} /* namespace so_5 */



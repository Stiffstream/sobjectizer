/*
	SObjectizer 5.
*/

/*!
 * \file
 * \brief Private interface for a coop.
 *
 * \since
 * v.5.6.0
 */

#pragma once

#include <so_5/coop.hpp>

namespace so_5
{

namespace impl
{

//
// coop_private_iface_t
//
/*!
 * \since
 * v.5.2.3
 *
 * \brief A special class for accessing private members of agent_coop.
 */
class coop_private_iface_t
{
	public :
#if 0
		static void
		do_deregistration_specific_actions(
			coop_t & coop,
			coop_dereg_reason_t dereg_reason )
		{
			coop.do_deregistration_specific_actions(
					std::move( dereg_reason ) );
		}

		static void
		do_registration_specific_actions(
			coop_t & coop,
			coop_t * parent_coop )
		{
			coop.do_registration_specific_actions( parent_coop );
		}

		static coop_reg_notificators_container_ref_t
		reg_notificators( const coop_t & coop )
		{
			return coop.reg_notificators();
		}

		static coop_dereg_notificators_container_ref_t
		dereg_notificators( const coop_t & coop )
		{
			return coop.dereg_notificators();
		}

		static coop_dereg_reason_t
		dereg_reason( const coop_t & coop )
		{
			return coop.dereg_reason();
		}
#endif

		static void
		increment_usage_count( coop_t & coop ) noexcept
		{
			coop.increment_usage_count();
		}

		static void
		decrement_usage_count( coop_t & coop )
		{
			coop.decrement_usage_count();
		}
};

} /* namespace impl */

} /* namespace so_5 */


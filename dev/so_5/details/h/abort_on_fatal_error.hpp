/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.5.6
 *
 * \file
 * \brief Helpers for aborting the whole application in the
 * case of a fatal error.
 */

#pragma once

#include <so_5/details/h/invoke_noexcept_code.hpp>

#include <cstdlib>

namespace so_5 {

namespace details {

/*!
 * \since
 * v.5.5.6
 *
 * \file
 * \brief Helper for aborting the whole application in the
 * case of a fatal error.
 *
 * \tparam L type of lambda with logging action. Note that this
 * lambda must not throws exceptions.
 *
 * \par Usage example
	\code
	if( coop_t::COOP_NOT_REGISTERED == m_agent_coop->m_registration_status )
		so_5::details::abort_on_fatal_error( [&] {
			SO_5_LOG_ERROR( so_environment(), log_stream )
			{
				log_stream << "Unexpected error: agent_coop has status "
					"COOP_NOT_REGISTERED at demand_handler_on_start. "
					"Cooperation: " << m_agent_coop->query_coop_name()
					<< ". Application will be aborted" << std::endl;
			}
		} );
	\endcode
 *
 */
template< typename L >
void
abort_on_fatal_error( L logging_lambda ) SO_5_NOEXCEPT
	{
		invoke_noexcept_code( std::move( logging_lambda ) );
		std::abort();
	}

} /* namespace details */

} /* namespace so_5 */


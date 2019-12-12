/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.4.0
 *
 * \file
 * \brief Helpers for handling unhandled exceptions from agent's event handlers.
 */

#include <so_5/impl/process_unhandled_exception.hpp>

#include <so_5/environment.hpp>

#include <so_5/details/abort_on_fatal_error.hpp>
#include <so_5/details/suppress_exceptions.hpp>

namespace so_5 {

namespace impl {

namespace {

/*!
 * \since
 * v.5.4.0
 *
 * \brief Switch agent to special state and deregister its cooperation.
 *
 * Calls abort() if an exception is raised during work.
 */
void
switch_agent_to_special_state_and_deregister_coop(
	//! Agent who is the producer of the exception.
	so_5::agent_t & a_exception_producer ) noexcept
	{
		const coop_handle_t coop = a_exception_producer.so_coop();
		try
		{
			a_exception_producer.so_switch_to_awaiting_deregistration_state();
			a_exception_producer.so_environment().deregister_coop(
					coop,
					so_5::dereg_reason::unhandled_exception );
		}
		catch( const std::exception & x )
		{
			so_5::details::abort_on_fatal_error( [&] {
				SO_5_LOG_ERROR( a_exception_producer.so_environment(), log_stream )
				{
					log_stream << "An exception '" << x.what()
							<< "' during deregistring cooperation "
							<< coop << " on unhandled exception"
							"processing. Application will be aborted.";
				}
			} );
		}
	}

/*!
 * \since
 * v.5.4.0
 *
 * \brief Switch agent to special state and initiate stopping
 * of SObjectizer Environment.
 *
 * Calls abort() if an exception is raised during work.
 */
void
switch_agent_to_special_state_and_shutdown_sobjectizer(
	//! Agent who is the producer of the exception.
	agent_t & a_exception_producer ) noexcept
	{
		try
		{
			a_exception_producer.so_switch_to_awaiting_deregistration_state();
			a_exception_producer.so_environment().stop();
		}
		catch( const std::exception & x )
		{
			so_5::details::abort_on_fatal_error( [&] {
				SO_5_LOG_ERROR( a_exception_producer.so_environment(), log_stream )
				{
					log_stream << "An exception '" << x.what()
							<< "' during shutting down SObjectizer on unhandled "
							"exception processing. Application will be aborted.";
				}
			} );
		}
	}

/*!
 * \since
 * v.5.4.0
 *
 * \brief Log unhandled exception from cooperation.
 *
 * \note
 * This function is noexcept since v.5.6.0.
 */
void
log_unhandled_exception(
	//! Raised and caught exception.
	const std::exception & ex_to_log,
	//! Agent who is the producer of the exception.
	agent_t & a_exception_producer ) noexcept
	{
		a_exception_producer.so_environment().call_exception_logger(
				ex_to_log,
				a_exception_producer.so_coop() );
	}

} /* namespace anonymous */

//
// process_unhandled_exception
//
void
process_unhandled_exception(
	current_thread_id_t working_thread_id,
	const std::exception & ex,
	agent_t & a_exception_producer ) noexcept
	{
		log_unhandled_exception( ex, a_exception_producer );

		auto reaction = a_exception_producer.so_exception_reaction();
		if( working_thread_id == null_current_thread_id() &&
				ignore_exception != reaction &&
				abort_on_exception != reaction )
		{
			so_5::details::abort_on_fatal_error( [&] {
				SO_5_LOG_ERROR( a_exception_producer.so_environment(), log_stream )
				{
					log_stream << "Illegal exception_reaction code "
							"for the multithreadded agent: "
							<< static_cast< int >(reaction) << ". "
							"The only allowed exception_reaction for "
							"such kind of agents are ignore_exception or "
							"abort_on_exception. "
							"Application will be aborted. "
							"Unhandled exception '" << ex.what()
							<< "' from cooperation "
							<< a_exception_producer.so_coop();
				}
			} );
		}

		if( abort_on_exception == reaction )
		{
			so_5::details::abort_on_fatal_error( [&] {
				SO_5_LOG_ERROR( a_exception_producer.so_environment(), log_stream )
				{
					log_stream << "Application will be aborted due to unhandled "
							"exception '" << ex.what() << "' from cooperation "
							<< a_exception_producer.so_coop();
				}
			} );
		}
		else if( shutdown_sobjectizer_on_exception == reaction )
		{
			// Since v.5.6.2 all logging-related exceptions are suppressed here.
			so_5::details::suppress_exceptions( [&] {
				SO_5_LOG_ERROR( a_exception_producer.so_environment(), log_stream )
				{
					log_stream << "SObjectizer will be shutted down due to "
							"unhandled exception '" << ex.what()
							<< "' from cooperation "
							<< a_exception_producer.so_coop();
				}
			} );

			switch_agent_to_special_state_and_shutdown_sobjectizer(
					a_exception_producer );
		}
		else if( deregister_coop_on_exception == reaction )
		{
			// Since v.5.6.2 all logging-related exceptions are suppressed here.
			so_5::details::suppress_exceptions( [&] {
				SO_5_LOG_ERROR( a_exception_producer.so_environment(), log_stream )
				{
					log_stream << "Cooperation "
							<< a_exception_producer.so_coop()
							<< " will be deregistered due to unhandled exception '"
							<< ex.what() << "'";
				}
			} );

			switch_agent_to_special_state_and_deregister_coop(
					a_exception_producer );
		}
		else if( ignore_exception == reaction )
		{
			// Since v.5.6.2 all logging-related exceptions are suppressed here.
			so_5::details::suppress_exceptions( [&] {
				SO_5_LOG_ERROR( a_exception_producer.so_environment(), log_stream )
				{
					log_stream << "Ignore unhandled exception '"
							<< ex.what() << "' from cooperation "
							<< a_exception_producer.so_coop();
				}
			} );
		}
		else
		{
			so_5::details::abort_on_fatal_error( [&] {
				SO_5_LOG_ERROR( a_exception_producer.so_environment(), log_stream )
				{
					log_stream << "Unknown exception_reaction code: "
							<< static_cast< int >(reaction)
							<< ". Application will be aborted. Unhandled exception '"
							<< ex.what() << "' from cooperation "
							<< a_exception_producer.so_coop();
				}
			} );
		}
	}

void
process_unhandled_unknown_exception(
	current_thread_id_t working_thread_id,
	agent_t & a_exception_producer ) noexcept
	{
		// Just call process_unhandled_exception with dummy exception object.
		exception_t dummy{
				"an exception of unknown type is caught",
				rc_unknown_exception_type
		};

		process_unhandled_exception(
				working_thread_id,
				dummy,
				a_exception_producer );
	}

} /* namespace impl */

} /* namespace so_5 */


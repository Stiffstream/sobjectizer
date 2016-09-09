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

#include <so_5/rt/impl/h/process_unhandled_exception.hpp>

#include <so_5/rt/h/environment.hpp>

#include <so_5/details/h/abort_on_fatal_error.hpp>

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
	so_5::agent_t & a_exception_producer )
	{
		const std::string coop_name = a_exception_producer.so_coop_name();
		try
		{
			a_exception_producer.so_switch_to_awaiting_deregistration_state();
			a_exception_producer.so_environment().deregister_coop(
					coop_name,
					so_5::dereg_reason::unhandled_exception );
		}
		catch( const std::exception & x )
		{
			so_5::details::abort_on_fatal_error( [&] {
				SO_5_LOG_ERROR( a_exception_producer.so_environment(), log_stream )
				{
					log_stream << "An exception '" << x.what()
							<< "' during deregistring cooperation '"
							<< coop_name << "' on unhandled exception"
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
	agent_t & a_exception_producer )
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
 * Calls abort() if an exception is raised during logging.
 */
void
log_unhandled_exception(
	//! Raised and caught exception.
	const std::exception & ex_to_log,
	//! Agent who is the producer of the exception.
	agent_t & a_exception_producer )
	{
		try
		{
			a_exception_producer.so_environment().call_exception_logger(
					ex_to_log,
					a_exception_producer.so_coop_name() );
		}
		catch( const std::exception & x )
		{
			so_5::details::abort_on_fatal_error( [&] {
				SO_5_LOG_ERROR( a_exception_producer.so_environment(), log_stream )
				{
					 log_stream << "An exception '" << x.what()
							<< "' during logging unhandled exception '"
							<< ex_to_log.what() << "' from cooperation '"
							<< a_exception_producer.so_coop_name()
							<< "'. Application will be aborted.";
				}
			} );
		}
	}

} /* namespace anonymous */

//
// process_unhandled_exception
//
void
process_unhandled_exception(
	current_thread_id_t working_thread_id,
	const std::exception & ex,
	agent_t & a_exception_producer )
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
							<< "' from cooperation '"
							<< a_exception_producer.so_coop_name() << "'";
				}
			} );
		}

		if( abort_on_exception == reaction )
		{
			so_5::details::abort_on_fatal_error( [&] {
				SO_5_LOG_ERROR( a_exception_producer.so_environment(), log_stream )
				{
					log_stream << "Application will be aborted due to unhandled "
							"exception '" << ex.what() << "' from cooperation '"
							<< a_exception_producer.so_coop_name() << "'";
				}
			} );
		}
		else if( shutdown_sobjectizer_on_exception == reaction )
		{
			SO_5_LOG_ERROR( a_exception_producer.so_environment(), log_stream )
			{
				log_stream << "SObjectizer will be shutted down due to "
						"unhandled exception '" << ex.what()
						<< "' from cooperation '"
						<< a_exception_producer.so_coop_name() << "'";
			}

			switch_agent_to_special_state_and_shutdown_sobjectizer(
					a_exception_producer );
		}
		else if( deregister_coop_on_exception == reaction )
		{
			SO_5_LOG_ERROR( a_exception_producer.so_environment(), log_stream )
			{
				log_stream << "Cooperation '"
						<< a_exception_producer.so_coop_name()
						<< "' will be deregistered due to unhandled exception '"
						<< ex.what() << "'";
			}

			switch_agent_to_special_state_and_deregister_coop(
					a_exception_producer );
		}
		else if( ignore_exception == reaction )
		{
			SO_5_LOG_ERROR( a_exception_producer.so_environment(), log_stream )
			{
				log_stream << "Ignore unhandled exception '"
						<< ex.what() << "' from cooperation '"
						<< a_exception_producer.so_coop_name() << "'";
			}
		}
		else
		{
			so_5::details::abort_on_fatal_error( [&] {
				SO_5_LOG_ERROR( a_exception_producer.so_environment(), log_stream )
				{
					log_stream << "Unknown exception_reaction code: "
							<< static_cast< int >(reaction)
							<< ". Application will be aborted. Unhandled exception '"
							<< ex.what() << "' from cooperation '"
							<< a_exception_producer.so_coop_name() << "'";
				}
			} );
		}
	}

} /* namespace impl */

} /* namespace so_5 */


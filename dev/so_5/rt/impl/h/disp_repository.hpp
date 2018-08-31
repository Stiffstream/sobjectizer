/*
	SObjectizer 5.
*/

/*!
	\file
	\brief A definition of utility class for work with dispatchers.
*/

#pragma once

#include <mutex>

#include <so_5/h/ret_code.hpp>
#include <so_5/h/spinlocks.hpp>

#include <so_5/rt/h/environment.hpp>
#include <so_5/rt/h/disp.hpp>

#include <so_5/rt/h/event_exception_logger.hpp>

namespace so_5
{

namespace impl
{

//
// disp_repository_t
//

//! A utility class for work with dispatchers.
class disp_repository_t
{
	public:
		disp_repository_t(
			environment_t & env,
			named_dispatcher_map_t named_dispatcher_map,
			event_exception_logger_unique_ptr_t logger );

		//! Get a named dispatcher.
		/*!
			\retval null-reference if dispatcher with \a disp_name
			has not been found.
		*/
		dispatcher_ref_t
		query_named_dispatcher(
			const std::string & disp_name );

		/*!
		 * \since
		 * v.5.4.0
		 *
		 * \brief Add named dispatcher if it is not exists.
		 */
		dispatcher_ref_t
		add_dispatcher_if_not_exists(
			const std::string & disp_name,
			std::function< dispatcher_unique_ptr_t() > disp_factory );

		//! Start all dispatchers.
		void
		start();

		//! Send a shutdown signal for all dispatchers and wait them
		//! to full stop.
		void
		finish();

		//! Install an exception logger.
		void
		install_exception_logger(
			event_exception_logger_unique_ptr_t logger );

		/*!
		 * \since
		 * v.5.2.3.
		 *
		 * \brief Call event exception logger for logging an exception.
		 */
		void
		call_exception_logger(
			//! Exception caught.
			const std::exception & event_exception,
			//! A cooperation to which agent is belong.
			const std::string & coop_name );

	private:
		/*!
		 * \since
		 * v.5.5.4
		 *
		 * \brief SObjectizer Environment to work in.
		 */
		environment_t & m_env;

		//! Named dispatchers.
		named_dispatcher_map_t m_named_dispatcher_map;

		//! Exception logger object lock.
		std::mutex m_exception_logger_lock;

		//! Exception logger.
		event_exception_logger_unique_ptr_t m_event_exception_logger;

		/*!
		 * \since
		 * v.5.4.0
		 *
		 * \brief State of dispatcher core.
		 */
		enum class state_t
		{
			//! Dispatcher core not started.
			not_started,
			//! Dispatcher core started.
			started,
			//! Dispatcher core is shutting down.
			finishing
		};

		/*!
		 * \since
		 * v.5.4.0
		 *
		 * \brief State of dispatcher core.
		 */
		state_t m_state;

		/*!
		 * \since
		 * v.5.4.0
		 *
		 * \brief Object lock.
		 */
		default_rw_spinlock_t m_lock;

		/*!
		 * \since
		 * v.5.2.0
		 *
		 * \brief Sends shutdown signal to all dispatchers.
		 */
		void
		send_shutdown_signal();

		/*!
		 * \since
		 * v.5.2.0
		 *
		 * \brief Wait for full stop of all dispatchers.
		 */
		void
		wait_for_full_shutdown();
};

} /* namespace impl */

} /* namespace so_5 */

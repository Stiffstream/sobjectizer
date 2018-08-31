/*
 * SObjectizer-5
 */

/*!
 * \file
 * \since
 * v.5.5.4
 *
 * \brief A standard implementation of controller for run-time monitoring.
 */

#pragma once

#include <so_5/rt/stats/h/controller.hpp>
#include <so_5/rt/stats/h/repository.hpp>

#include <condition_variable>
#include <mutex>
#include <thread>
#include <memory>
#include <chrono>

namespace so_5
{

namespace stats
{

namespace impl
{

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnon-virtual-dtor"
#endif

/*!
 * \since
 * v.5.5.4
 *
 * \brief A standard implementation of controller for run-time monitoring.
 */
class std_controller_t final
	:	public controller_t
	,	public repository_t
	{
	public :
		std_controller_t( mbox_t mbox );

		// Implementation of controller_t interface.
		virtual const mbox_t &
		mbox() const override;

		virtual void
		turn_on() override;

		virtual void
		turn_off() override;

		virtual std::chrono::steady_clock::duration
		set_distribution_period(
			std::chrono::steady_clock::duration period ) override;

		// Implementation of repository_t interface.
		virtual void
		add( source_t & what ) override;

		virtual void
		remove( source_t & what ) override;

	private :
		//! Mbox for sending monitoring data.
		const mbox_t m_mbox;

		//! Object lock for start/stop operations.
		std::mutex m_start_stop_lock;
		//! Object lock for data-related operations.
		std::mutex m_data_lock;
		//! Condition for wake-up data-distribution thread.
		std::condition_variable m_wake_up_cond;

		//! Main data-distribution thread.
		/*!
		 * This thread is created in turn_on() and is destroyed in turn_off().
		 * It is not exists if run-time monitoring is switched off.
		 */
		std::unique_ptr< std::thread > m_distribution_thread;

		//! Shutdown signal.
		/*!
		 * It is set to true in turn_off().
		 */
		bool m_shutdown_initiated = { false };

		/*!
		 * \name Data sources-related part of controller's data.
		 * \{
		 */
		//! Head of data sources list.
		source_t * m_head = { nullptr };
		//! Tail of data sources list.
		source_t * m_tail = { nullptr };

		//! Data-distribution period.
		std::chrono::steady_clock::duration m_distribution_period =
				{ default_distribution_period() };
		/*!
		 * \}
		 */

		//! Main body of data distribution thread.
		void
		body();

		//! Initiates distribution of current values for all data sources.
		/*!
		 * \return A time spent for data distribution.
		 */
		std::chrono::steady_clock::duration
		distribute_current_data();
	};

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

} /* namespace impl */

} /* namespace stats */

} /* namespace so_5 */


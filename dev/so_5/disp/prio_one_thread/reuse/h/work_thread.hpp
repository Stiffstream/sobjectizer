/*
	SObjectizer 5.
*/

/*!
 * \file
 * \brief A working thread for dispatcher with one common working
 * thread and support of demands priority.
 *
 * \since
 * v.5.5.8
 */

#pragma once

#include <so_5/h/current_thread_id.hpp>

#include <so_5/rt/stats/h/work_thread_activity.hpp>
#include <so_5/rt/stats/impl/h/activity_tracking.hpp>

#include <so_5/details/h/at_scope_exit.hpp>

#include <thread>

namespace so_5 {

namespace disp {

namespace prio_one_thread {

namespace reuse {

namespace work_thread_details {

//
// common_data_t
//
/*!
 * \brief A common data for all work thread implementations.
 *
 * \since
 * v.5.5.18
 */
template< typename Demand_Queue >
struct common_data_t
	{
		//! Demands queue to work for.
		Demand_Queue & m_queue;

		//! Thread object.
		std::thread m_thread;

		//! ID of the work thread.
		/*!
		 * \note Receives actual value only after successful start
		 * of the thread.
		 */
		so_5::current_thread_id_t m_thread_id;

		common_data_t( Demand_Queue & queue ) : m_queue( queue ) {}
	};

//
// no_activity_tracking_impl_t
//
/*!
 * \brief A part of implementation of work thread without activity tracking.
 *
 * \since
 * v.5.5.18
 */
template< typename Demand_Queue >
class no_activity_tracking_impl_t : protected common_data_t< Demand_Queue >
	{
		using base_type_t = common_data_t< Demand_Queue >;

	public :
		no_activity_tracking_impl_t( Demand_Queue & queue )
			:	base_type_t( queue )
			{}

	protected :
		void
		work_started() { /* Nothing to do. */ }

		void
		work_finished() { /* Nothing to do. */ }

		void
		wait_started() { /* Nothing to do. */ }

		void
		wait_finished() { /* Nothing to do. */ }
	};

//
// with_activity_tracking_impl_t
//
/*!
 * \brief A part of implementation of work thread with activity tracking.
 *
 * \since
 * v.5.5.18
 */
template< typename Demand_Queue >
class with_activity_tracking_impl_t : protected common_data_t< Demand_Queue >
	{
		using base_type_t = common_data_t< Demand_Queue >;

	public :
		with_activity_tracking_impl_t( Demand_Queue & queue )
			:	base_type_t( queue )
			{}

		so_5::stats::work_thread_activity_stats_t
		take_activity_stats()
			{
				so_5::stats::work_thread_activity_stats_t result;

				result.m_working_stats = m_working_stats.take_stats();
				result.m_waiting_stats = m_waiting_stats.take_stats();

				return result;
			}

	protected :
		//! Statictics for work activity.
		so_5::stats::activity_tracking_stuff::stats_collector_t<
				so_5::stats::activity_tracking_stuff::internal_lock >
			m_working_stats;

		//! Statictics for wait activity.
		so_5::stats::activity_tracking_stuff::stats_collector_t<
				so_5::stats::activity_tracking_stuff::internal_lock >
			m_waiting_stats;

		void
		work_started() { m_working_stats.start(); }

		void
		work_finished() { m_working_stats.stop(); }

		void
		wait_started() { m_waiting_stats.start(); }

		void
		wait_finished() { m_waiting_stats.stop(); }
	};

} /* namespace work_thread_details */

//
// work_thread_template_t
//
/*!
 * \brief A working thread for dispatcher with one common working
 * thread and support of demands priority.
 *
 * \since
 * v.5.5.8, v.5.5.18
 */
template<
	typename Demand_Queue,
	template<class> class Work_Thread >
class work_thread_template_t : public Work_Thread< Demand_Queue >
	{
		using base_type_t = Work_Thread< Demand_Queue >;

	public :
		//! Initializing constructor.
		work_thread_template_t( Demand_Queue & queue )
			:	base_type_t( queue )
			{}

		void
		start()
			{
				this->m_thread = std::thread( [this]() { body(); } );
			}

		void
		join()
			{
				this->m_thread.join();
			}

		so_5::current_thread_id_t
		thread_id() const
			{
				return this->m_thread_id;
			}

	private :
		void
		body()
			{
				this->m_thread_id = so_5::query_current_thread_id();

				try
					{
						for(;;)
							{
								auto d = this->pop_demand();
								this->call_handler( *d );
							}
					}
				catch( const typename Demand_Queue::shutdown_ex_t & )
					{}
			}

		auto
		pop_demand() -> decltype(std::declval<Demand_Queue>().pop())
			{
				this->wait_started();
				auto wait_meter_stopper = so_5::details::at_scope_exit(
						[this] { this->wait_finished(); } );

				return this->m_queue.pop();
			}

		void
		call_handler( so_5::execution_demand_t & demand )
			{
				this->work_started();
				auto work_meter_stopper = so_5::details::at_scope_exit(
						[this] { this->work_finished(); } );

				demand.call_handler( this->m_thread_id );
			}
	};

//
// work_thread_no_activity_tracking_t
//
template< typename Demand_Queue >
using work_thread_no_activity_tracking_t =
	work_thread_template_t<
			Demand_Queue,
			work_thread_details::no_activity_tracking_impl_t >;

//
// work_thread_with_activity_tracking_t
//
template< typename Demand_Queue >
using work_thread_with_activity_tracking_t =
	work_thread_template_t<
			Demand_Queue,
			work_thread_details::with_activity_tracking_impl_t >;

} /* namespace reuse */

} /* namespace prio_one_thread */

} /* namespace disp */

} /* namespace so_5 */


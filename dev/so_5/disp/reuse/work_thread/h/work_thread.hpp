/*
	SObjectizer 5.
*/

/*!
	\file
	\brief Working thread for dispatchers.
*/

#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>

#include <so_5/h/declspec.hpp>
#include <so_5/h/current_thread_id.hpp>

#include <so_5/rt/h/event_queue.hpp>

#include <so_5/disp/mpsc_queue_traits/h/pub.hpp>

#include <so_5/rt/stats/h/work_thread_activity.hpp>
#include <so_5/rt/stats/impl/h/activity_tracking.hpp>

namespace so_5
{

namespace disp
{

namespace reuse
{

namespace work_thread
{

//! Typedef for demand's container.
typedef std::deque< execution_demand_t > demand_container_t;

namespace queue_traits = so_5::disp::mpsc_queue_traits;

//! Typedef for atomic demands counter.
/*!
 * \since
 * v.5.5.7
 */
using demands_counter_t = std::atomic< std::size_t >;

/*!
 * \brief Type for result of demand extraction.
 */
enum class extraction_result_t
{
	//! Demand has been extracted.
	demand_extracted = 1,
	//! Demand has not been extracted because of shutdown.
	shutting_down = 2,
	//! Demand has not been extracted because the demand queue is empty.
	no_demands = 3
};

namespace demand_queue_details
{

/*!
 * \brief Common data for all implementations of demand_queue.
 *
 * \since 
 * v.5.5.18
 */
struct common_data_t
{
	//! Demand queue.
	demand_container_t m_demands;

	//! \name Objects for the thread safety.
	//! \{
	queue_traits::lock_unique_ptr_t m_lock;
	//! \}

	//! Service flag.
	/*!
		true -- shall do the service, methods push/pop must work.
		false -- the service is stopped or will be stopped.
	*/
	bool m_in_service{ false };

	//! Initializing constructor.
	common_data_t(
		//! Lock object to be used by queue.
		queue_traits::lock_unique_ptr_t lock )
		:	m_lock( std::move(lock) )
	{}

	~common_data_t()
	{
		m_demands.clear();
	}
};

/*!
 * \brief A part of demand queue implementation for the case
 * when activity tracking is not used.
 */
class no_activity_tracking_impl_t : protected common_data_t
{
public :
	no_activity_tracking_impl_t(
		queue_traits::lock_unique_ptr_t lock )
		:	common_data_t( std::move(lock) )
	{}

protected :
	void
	wait_started() {}

	void 
	wait_finished() {}
};

/*!
 * \brief A part of demand queue implementation for the case
 * when activity tracking is used.
 */
class with_activity_tracking_impl_t : protected common_data_t
{
public :
	with_activity_tracking_impl_t(
		queue_traits::lock_unique_ptr_t lock )
		:	common_data_t( std::move(lock) )
		,	m_waiting_stats( *m_lock )
	{}

	so_5::stats::activity_stats_t
	take_activity_stats()
	{
		return m_waiting_stats.take_stats();
	}

protected :
	void
	wait_started()
	{
		m_waiting_stats.start();
	}

	void 
	wait_finished()
	{
		m_waiting_stats.stop();
	}

private :
	so_5::stats::activity_tracking_stuff::stats_collector_t<
			so_5::stats::activity_tracking_stuff::external_lock<
					queue_traits::lock_t,
					so_5::stats::activity_tracking_stuff::no_lock_at_start_stop_policy > >
		m_waiting_stats;
};

//
// demand_queue_template_t
//

//! Implementation of demand_queue in form of a template.
/*!
	demand_queue_t has shutdown flag inside.

	demand_queue_t is thread safe and is intended to be used by 
	several concurrent threads.
*/
template< typename Impl >
class queue_template_t
	:	public event_queue_t
	,	public Impl
{
public:
	queue_template_t(
		//! Lock object to be used by queue.
		queue_traits::lock_unique_ptr_t lock )
		:	Impl( std::move(lock) )
	{}

	/*!
	 * \name Implementation of event_queue interface.
	 * \{
	 */
	virtual void
	push( execution_demand_t demand ) override
	{
		queue_traits::lock_guard_t guard{ *(this->m_lock) };

		if( this->m_in_service )
		{
			const bool demands_empty_before_service = this->m_demands.empty();

			this->m_demands.push_back( std::move( demand ) );

			if( demands_empty_before_service )
			{
				// May be someone is waiting...
				// It should be informed about new demands.
				guard.notify_one();
			}
		}
	}
	/*!
	 * \}
	 */

	//! Try to extract demands from the queue.
	/*!
		If there is no demands in queue then current thread
		will sleep until:
		- the new demand is put in the queue;
		- a shutdown signal.

		\note Since v.5.5.7 this method also updates external demands
		counter. This update is performed under queue's lock.
		It should prevent errors when run-time monitor can get wrong
		quantity of demands.
	*/
	extraction_result_t
	pop(
		/*! Receiver for extracted demands. */
		demand_container_t & demands,
		/*! External demands counter to be updated. */
		demands_counter_t & external_counter )
	{
		queue_traits::unique_lock_t lock{ *(this->m_lock) };
		while( true )
		{
			if( this->m_in_service && !this->m_demands.empty() )
			{
				demands.swap( this->m_demands );

				// It's time to update external counter.
				external_counter.store( demands.size(), std::memory_order_release );

				break;
			}
			else if( !this->m_in_service )
				return extraction_result_t::shutting_down;
			else
			{
				// Queue is empty. We should wait for a demand or
				// a shutdown signal.

				// Since v.5.5.18 we must take care about activity tracking.
				this->wait_started();

				lock.wait_for_notify();

				this->wait_finished();
			}
		}

		return extraction_result_t::demand_extracted;
	}

	//! Start demands processing.
	void
	start_service()
	{
		queue_traits::lock_guard_t lock{ *(this->m_lock) };

		this->m_in_service = true;
	}

	//! Stop demands processing.
	void
	stop_service()
	{
		queue_traits::lock_guard_t lock{ *(this->m_lock) };

		this->m_in_service = false;
		// If the demands queue is empty then someone is waiting
		// for new demands inside pop().
		if( this->m_demands.empty() )
			lock.notify_one();
	}

	//! Clear demands queue.
	void
	clear()
	{
		queue_traits::lock_guard_t lock{ *(this->m_lock) };

		this->m_demands.clear();
	}

	/*!
	 * \since
	 * v.5.5.4
	 *
	 * \brief Get the count of demands in the queue.
	 *
	 * \note Since v.5.5.7 this method also uses external demands
	 * counter. Addition of demands quantity inside demands queue and
	 * the value of external counter is performed under the queue lock.
	 */
	std::size_t
	demands_count( const demands_counter_t & external_counter )
	{
		queue_traits::lock_guard_t lock{ *(this->m_lock) };

		return this->m_demands.size()
				+ external_counter.load( std::memory_order_acquire );
	}
};

} /* namespace demand_queue_details */

/*!
 * \brief An alias for demand_queue without activity tracking.
 *
 * \since
 * v.5.5.18
 */
using demand_queue_no_activity_tracking_t =
	demand_queue_details::queue_template_t<
		demand_queue_details::no_activity_tracking_impl_t >;

/*!
 * \brief An alias for demand_queue with activity tracking.
 *
 * \since
 * v.5.5.18
 */
using demand_queue_with_activity_tracking_t =
	demand_queue_details::queue_template_t<
		demand_queue_details::with_activity_tracking_impl_t >;

namespace details
{

//! Thread status flag.
enum class status_t : int
{
	//! 0 - thread execution should be stopped.
	stopped = 0,
	//! 1 - thread execution should be continued.
	working = 1
};

/*!
 * \brief Common data for all work thread implementations.
 *
 * \since
 * v.5.5.18
 */
template< typename Demand_Queue >
struct common_data_t
{
	//! Working thread.
	std::thread m_thread;

	//! Thread status flag.
	std::atomic< status_t > m_status{ status_t::stopped };

	//! Demands queue.
	Demand_Queue m_queue;

	/*!
	 * \brief ID of working thread.
	 *
	 * \attention Receive the value only after start of thread body.
	 */
	so_5::current_thread_id_t m_thread_id;

	/*!
	 * \brief A counter for calculating count of demands in
	 * the queue.
	 *
	 * \note Will be used for run-time monitoring.
	 */
	demands_counter_t m_demands_count = { 0 };

	common_data_t(
		queue_traits::lock_factory_t queue_lock_factory )
		:	m_queue( queue_lock_factory() )
	{}
};

/*!
 * \brief Part of implementation of work thread without activity tracking.
 *
 * \since
 * v.5.5.18
 */
class no_activity_tracking_impl_t
	: protected common_data_t< demand_queue_no_activity_tracking_t >
{
public :
	no_activity_tracking_impl_t(
		queue_traits::lock_factory_t queue_lock_factory )
		:	common_data_t( std::move(queue_lock_factory) )
	{}

protected :
	//! Main method for serving block of demands.
	void
	serve_demands_block(
		//! Bunch of demands to be processed.
		demand_container_t & demands )
	{
		while( !demands.empty() )
		{
			auto & demand = demands.front();

			demand.call_handler( this->m_thread_id );

			demands.pop_front();
			--(this->m_demands_count);
		}
	}
};

/*!
 * \brief Part of implementation of work thread with activity tracking.
 *
 * \since
 * v.5.5.18
 */
class activity_tracking_impl_t
	: protected common_data_t< demand_queue_with_activity_tracking_t >
{
	using activity_tracking_traits = so_5::stats::activity_tracking_stuff::traits;

public :
	activity_tracking_impl_t(
		queue_traits::lock_factory_t queue_lock_factory )
		:	common_data_t( std::move(queue_lock_factory) )
	{}

	/*!
	 * \brief Get the activity stats.
	 */
	so_5::stats::work_thread_activity_stats_t
	take_activity_stats()
	{
		namespace stats = so_5::stats;

		stats::work_thread_activity_stats_t result;
		bool is_working{ false };
		stats::clock_type_t::time_point working_started_at;

		{
			std::lock_guard< activity_tracking_traits::lock_t > lock{ m_stats_lock };
			result.m_working_stats = m_activity_stats;

			// Special care to the current activity (if exists).
			if( m_activity_started_at )
			{
				is_working = true;
				working_started_at = *m_activity_started_at;
			}
		}

		if( is_working )
		{
			stats::details::update_stats_from_current_time(
					result.m_working_stats,
					working_started_at );
		}

		result.m_waiting_stats = m_queue.take_activity_stats();

		return result;
	}

protected :
	//! Main method for serving block of demands.
	void
	serve_demands_block(
		//! Bunch of demands to be processed.
		demand_container_t & demands )
	{
		auto activity_started_at = so_5::stats::clock_type_t::now();

		{
			std::lock_guard< activity_tracking_traits::lock_t > lock{ m_stats_lock };
			m_activity_started_at = &activity_started_at;
			m_activity_stats.m_count += 1;
		}

		while( !demands.empty() )
		{
			auto & demand = demands.front();

			demand.call_handler( m_thread_id );

			const auto activity_finished_at = so_5::stats::clock_type_t::now();

			demands.pop_front();
			--m_demands_count;

			{
				std::lock_guard< activity_tracking_traits::lock_t > lock{ m_stats_lock };
				so_5::stats::details::update_stats_from_duration(
						m_activity_stats,
						activity_finished_at - activity_started_at );

				if( demands.empty() )
					m_activity_started_at = nullptr;
				else
				{
					activity_started_at = activity_finished_at;
					m_activity_stats.m_count += 1;
				}
			}
		}
	}

private :
	/*!
	 * \brief Lock for manipulation of activity stats.
	 */
	activity_tracking_traits::lock_t m_stats_lock;

	/*!
	 * \brief Pointer to time_point object related to the current activity.
	 *
	 * Value nullptr means that there is no any event running on
	 * the work thread.
	 */
	const so_5::stats::clock_type_t::time_point * m_activity_started_at{};

	/*!
	 * \brief Activity statistics.
	 */
	so_5::stats::activity_stats_t m_activity_stats{};
};

/*!
 * \brief Implementation of work thread in form of template.
 *
 * \since
 * v.5.5.18
 */
template< typename Impl >
class work_thread_template_t : public Impl
{
public :
	work_thread_template_t(
		//! Factory for creation of lock object for demand queue.
		queue_traits::lock_factory_t queue_lock_factory )
		:	Impl( std::move(queue_lock_factory) )
	{}

	//! Start the working thread.
	void
	start()
	{
		this->m_queue.start_service();
		this->m_status = status_t::working;

		this->m_thread = std::thread( [this]() { this->body(); } );
	}

	//! Send the shutdown signal to the working thread.
	void
	shutdown()
	{
		this->m_status = status_t::stopped;
		this->m_queue.stop_service();
	}

	//! Wait the full stop of the working thread.
	/*!
	 * All non-processed demands from the queue will be destroyed
	 * after a stop of the working thread.
	 */
	void
	wait()
	{
		this->m_thread.join();
		this->m_queue.clear();
	}

	/*!
	 * \brief Get the underlying event_queue object.
	 */
	event_queue_t &
	event_queue()
	{
		return this->m_queue;
	}

	/*!
	 * \brief Get a binding information for an agent.
	 */
	event_queue_t *
	get_agent_binding()
	{
		return &this->event_queue();
	}

	/*!
	 * \brief Get the count of demands in the queue.
	 */
	std::size_t
	demands_count()
	{
		return this->m_queue.demands_count( this->m_demands_count );
	}

	/*!
	 * \brief Get ID of work thread.
	 *
	 * \note This method returns correct value only after start
	 * of the thread.
	 *
	 * \since
	 * v.5.5.18
	 */
	so_5::current_thread_id_t
	thread_id() const
	{
		return this->m_thread_id;
	}

private :
	//! Main thread body.
	void
	body()
	{
		// Store current thread ID to attribute to avoid thread ID
		// request on every event execution.
		this->m_thread_id = so_5::query_current_thread_id();

		// Local demands queue.
		demand_container_t demands;

		auto result = extraction_result_t::no_demands;

		while( status_t::working == this->m_status )
		{
			// If the local queue is empty then we should try
			// to get new demands.
			if( demands.empty() )
				result = this->m_queue.pop( demands, this->m_demands_count );

			// Serve demands if any.
			if( extraction_result_t::demand_extracted == result )
				this->serve_demands_block( demands );
		}
	}
};

} /* namespace details */

//
// work_thread_t
//

/*!
 * \brief Type of work thread without activity tracking.
 */
using work_thread_no_activity_tracking_t =
	details::work_thread_template_t<
		details::no_activity_tracking_impl_t >;

//
// work_thread_with_activity_tracking_t
//

//! Working thread with activity tracking.
/*!
 * \since
 * v.5.5.18
 */
using work_thread_with_activity_tracking_t =
	details::work_thread_template_t<
		details::activity_tracking_impl_t >;

} /* namespace work_thread */

} /* namespace reuse */

} /* namespace disp */

} /* namespace so_5 */


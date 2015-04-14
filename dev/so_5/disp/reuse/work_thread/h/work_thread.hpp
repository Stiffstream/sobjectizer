/*
	SObjectizer 5.
*/

/*!
	\file
	\brief Working thread for dispatchers.
*/

#if !defined( _SO_5__DISP__REUSE__WORK_THREAD__WORK_THREAD_HPP_ )
#define _SO_5__DISP__REUSE__WORK_THREAD__WORK_THREAD_HPP_

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

#include <so_5/h/spinlocks.hpp>

#include <so_5/disp/reuse/locks/h/locks.hpp>

namespace so_5
{

namespace disp
{

namespace reuse
{

namespace work_thread
{

//! Typedef for demand's container.
typedef std::deque< so_5::rt::execution_demand_t > demand_container_t;

using queue_lock_t = so_5::disp::reuse::locks::combined_queue_lock_t;
using queue_unique_lock_t =
		so_5::disp::reuse::locks::combined_queue_unique_lock_t;
using queue_lock_guard_t =
		so_5::disp::reuse::locks::combined_queue_lock_guard_t;

//
// demand_queue_t
//

//! Queue of demands to process agent events.
/*!
	demand_queue_t has shutdown flag inside.

	demand_queue_t is thread safe and is intended to be used by 
	several concurrent threads.
*/
class demand_queue_t : public so_5::rt::event_queue_t
{
	public:
		demand_queue_t();
		~demand_queue_t();

		/*!
		 * \name Implementation of event_queue interface.
		 * \{
		 */
		virtual void
		push(
			so_5::rt::execution_demand_t demand );
		/*!
		 * \}
		 */
		enum
		{
			//! Demand has been extracted.
			demand_extracted = 1,
			//! Demand has not been extracted because of shutdown.
			shutting_down = 2,
			//! Demand has not been extracted because the demand queue is empty.
			no_demands = 3
		};

		//! Try to extract demands from the queue.
		/*!
			If there is no demands in queue then current thread
			will sleep until:
			- the new demand is put in the queue;
			- a shutdown signal.
		*/
		int
		pop(
			/*! Receiver for extracted demands. */
			demand_container_t & queue_item );

		//! Start demands processing.
		void
		start_service();

		//! Stop demands processing.
		void
		stop_service();

		//! Clear demands queue.
		void
		clear();

		/*!
		 * \since v.5.5.4
		 * \brief Get the count of demands in the queue.
		 */
		std::size_t
		demands_count();

	private:
		//! Demand queue.
		demand_container_t m_demands;

		//! \name Objects for the thread safety.
		//! \{
		queue_lock_t m_lock;
		//! \}

		//! Service flag.
		/*!
			true -- shall do the service, methods push/pop must work.
			false -- the service is stopped or will be stopped.
		*/
		bool m_in_service;
};

//
// work_thread_t
//

//! Working thread.
/*!
 * Working thread should be used inside of some dispatcher.
 * And the lifetime of the dispatcher object must be longer than
 * the lifetime of the working thread object.
 */
class work_thread_t
{
	public:
		work_thread_t();

		//! Start the working thread.
		void
		start();

		//! Send the shutdown signal to the working thread.
		void
		shutdown();

		//! Wait the full stop of the working thread.
		/*!
		 * All non-processed demands from the queue will be destroyed
		 * after a stop of the working thread.
		 */
		void
		wait();

		/*!
		 * \since v.5.4.0
		 * \brief Get the underlying event_queue object.
		 */
		so_5::rt::event_queue_t &
		event_queue();

		/*!
		 * \since v.5.4.0
		 * \brief Get a binding information for an agent.
		 */
		so_5::rt::event_queue_t *
		get_agent_binding();

		/*!
		 * \since v.5.5.4
		 * \brief Get the count of demands in the queue.
		 */
		std::size_t
		demands_count();

	protected:
		//! Main working thread body.
		void
		body();

		//! Handle a bunch of demands.
		void
		serve_demands_block(
			//! Bunch of demands to be processed.
			demand_container_t & executed_demands );

	private:
		//! Demands queue.
		demand_queue_t m_queue;

		//! Thread status flag.
		enum
		{
			//! 0 - thread execution should be stopped.
			WORK_THREAD_STOP = 0,
			//! 1 - thread execution should be continued.
			WORK_THREAD_CONTINUE = 1
		};

		//! Thread status flag.
		/*!
		 * Available values are: WORK_THREAD_STOP, WORK_THREAD_CONTINUE
		 */
		std::atomic_long m_continue_work;

		//! Actual working thread.
		std::unique_ptr< std::thread > m_thread;

		/*!
		 * \since v.5.4.0
		 * \brief ID of working thread.
		 *
		 * \attention Receive the value only after start of thread body.
		 */
		so_5::current_thread_id_t m_thread_id;

		/*!
		 * \since v.5.5.4
		 * \brief A counter for calculating count of demands in
		 * the queue.
		 *
		 * \note Will be used for run-time monitoring.
		 */
		std::atomic< std::size_t > m_demands_count = { 0 };
};

/*!
 * \since v.5.4.0
 * \brief Shared pointer for work_thread.
 */
typedef std::shared_ptr< work_thread_t > work_thread_shptr_t;

} /* namespace work_thread */

} /* namespace reuse */

} /* namespace disp */

} /* namespace so_5 */

#endif


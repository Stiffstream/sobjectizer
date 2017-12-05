/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief An implementation of advanced thread pool dispatcher.
 *
 * \since
 * v.5.4.0
 */

#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <memory>
#include <map>
#include <iostream>
#include <forward_list>

#include <so_5/h/spinlocks.hpp>
#include <so_5/h/atomic_refcounted.hpp>

#include <so_5/rt/h/event_queue.hpp>
#include <so_5/rt/h/disp.hpp>

#include <so_5/rt/stats/impl/h/activity_tracking.hpp>

#include <so_5/disp/reuse/h/mpmc_ptr_queue.hpp>

#include <so_5/disp/thread_pool/impl/h/common_implementation.hpp>

#if 0
	#define SO_5_CHECK_INVARIANT_IMPL(what, data, file, line) \
	if( !(what) ) { \
		std::cerr << file << ":" << line << ": FAILED INVARIANT: " << #what << "; data: " << data << std::endl; \
		std::abort(); \
	}
	#define SO_5_CHECK_INVARIANT(what, data) SO_5_CHECK_INVARIANT_IMPL(what, data, __FILE__, __LINE__)
#else
	#define SO_5_CHECK_INVARIANT(what, data)
#endif


namespace so_5
{

namespace disp
{

namespace adv_thread_pool
{

namespace impl
{

using spinlock_t = so_5::default_spinlock_t;

class agent_queue_t;

namespace stats = so_5::stats;
namespace tp_stats = so_5::disp::reuse::thread_pool_stats;

//
// dispatcher_queue_t
//
using dispatcher_queue_t = so_5::disp::reuse::mpmc_ptr_queue_t< agent_queue_t >;

//
// agent_queue_t
//
/*!
 * \brief Event queue for the agent (or cooperation).
 *
 * \since
 * v.5.4.0
 */
class agent_queue_t
	:	public event_queue_t
	,	private so_5::atomic_refcounted_t
	{
		friend class so_5::intrusive_ptr_t< agent_queue_t >;

	private :
		//! Actual demand in event queue.
		struct demand_t
			{
				//! Actual demand.
				execution_demand_t m_demand;

				//! Next item in queue.
				demand_t * m_next;

				demand_t()
					:	m_next( nullptr )
					{}
				demand_t( execution_demand_t && original )
					:	m_demand( std::move( original ) )
					,	m_next( nullptr )
					{}
			};

	public :
		static const unsigned int thread_safe_worker = 2;
		static const unsigned int not_thread_safe_worker = 1;

		//! Constructor.
		agent_queue_t(
			//! Dispatcher queue to work with.
			dispatcher_queue_t & disp_queue,
			//! Dummy argument. It is necessary here because of
			//! common implementation for thread-pool and
			//! adv-thread-pool dispatchers.
			const params_t & )
			:	m_disp_queue( disp_queue )
			,	m_tail( &m_head )
			,	m_active( false )
			,	m_workers( 0 )
			{}

		~agent_queue_t()
			{
				while( m_head.m_next )
					delete_head();
			}

		//! Access to the queue's lock.
		spinlock_t &
		lock()
			{
				return m_lock;
			}

		//! Push next demand to queue.
		virtual void
		push( execution_demand_t demand )
			{
				bool need_schedule = false;
				{
					// Do memory allocation before spinlock locking.
					auto new_demand = new demand_t( std::move( demand ) );

					std::lock_guard< spinlock_t > lock( m_lock );

					m_tail->m_next = new_demand;
					m_tail = m_tail->m_next;

					++m_size;

					if( m_head.m_next == m_tail )
						{
							// Queue was empty. Need to detect
							// necessity of queue activation.
							if( !m_active )
								if( !is_there_not_thread_safe_worker() )
								{
									need_schedule = true;
									m_active = true;
								}
						}

					SO_5_CHECK_INVARIANT( !empty(), this )
					SO_5_CHECK_INVARIANT( m_active || is_there_any_worker(), this )
					SO_5_CHECK_INVARIANT( !(need_schedule && !m_active), this )
				}

				if( need_schedule )
					m_disp_queue.schedule( this );
			}

		//! Get the information about the front demand.
		/*!
		 * \attention This method must be called only on non-empty queue.
		 */
		execution_demand_t
		peek_front()
			{
				SO_5_CHECK_INVARIANT( !empty(), this )
				SO_5_CHECK_INVARIANT( m_active, this )

				m_active = false;

				return m_head.m_next->m_demand;
			}

		//! Remove the front demand.
		/*!
		 * \retval true queue must be activated.
		 * \retval false queue must not be activated.
		 */
		bool
		worker_started(
			//! Type of worker.
			//! Must be thread_safe_worker or not_thread_safe_worker.
			unsigned int type_of_worker )
			{
				SO_5_CHECK_INVARIANT( !empty(), this );
				SO_5_CHECK_INVARIANT( !m_active, this );

				delete_head();
				if( !m_head.m_next )
					m_tail = &m_head;

				m_workers += type_of_worker;

				// Queue must be activated only if queue is not empty
				// and current worker is a thread safe worker.
				m_active = ( !empty() &&
						thread_safe_worker == type_of_worker );

				return m_active;
			}

		//! Signal about finishing of worker of the specified type.
		/*!
		 * \retval true queue must be activated.
		 * \retval false queue must not be activated.
		 */
		bool
		worker_finished(
			//! Type of worker.
			//! Must be thread_safe_worker or not_thread_safe_worker.
			unsigned int type_of_worker )
			{
				m_workers -= type_of_worker;

				bool old_active = m_active;
				if( !m_active )
					m_active = !empty();

				SO_5_CHECK_INVARIANT( !(m_active && empty()), this )
				SO_5_CHECK_INVARIANT(
						!old_active || m_active, this );

				return old_active != m_active;
			}

		//! Check the presence of any worker at the moment.
		bool
		is_there_any_worker() const
			{
				return 0 != m_workers;
			}

		//! Check the presence of thread unsafe worker.
		bool
		is_there_not_thread_safe_worker() const
			{
				return 0 != (m_workers & not_thread_safe_worker );
			}

		//! Is empty queue?
		bool
		empty() const { return nullptr == m_head.m_next; }

		//! Is active queue?
		bool
		active() const { return m_active; }

		/*!
		 * \brief Get the current size of the queue.
		 *
		 * \since
		 * v.5.5.4
		 */
		std::size_t
		size() const
			{
				return m_size.load( std::memory_order_acquire );
			}

	private :
		//! Dispatcher queue for scheduling processing of events from
		//! this queue.
		dispatcher_queue_t & m_disp_queue;

		//! Object's lock.
		spinlock_t m_lock;

		//! Head of the demand's queue.
		/*!
		 * Never contains actual demand. Only m_next field is used.
		 */
		demand_t m_head;
		//! Tail of the demand's queue.
		/*!
		 * Must point to m_head if queue is empty or to the very
		 * last queue item otherwise.
		 */
		demand_t * m_tail;

		//! Is this queue activated?
		/*!
		 * Queue is activated if it is scheduled to dispatcher queue.
		 */
		bool m_active;

		//! Count of active workers.
		unsigned int m_workers;

		/*!
		 * \brief Current size of the queue.
		 *
		 * \since
		 * v.5.5.4
		 */
		std::atomic< std::size_t > m_size = { 0 };

		//! Helper method for deleting queue's head object.
		inline void
		delete_head()
			{
				auto to_be_deleted = m_head.m_next;
				m_head.m_next = m_head.m_next->m_next;

				--m_size;

				delete to_be_deleted;
			}
	};

//
// agent_queue_ref_t
//
/*!
 * \brief A typedef of smart pointer for agent_queue.
 *
 * \since
 * v.5.4.0
 */
typedef so_5::intrusive_ptr_t< agent_queue_t > agent_queue_ref_t;

namespace work_thread_details {

/*!
 * \brief Main data for work_thread.
 *
 * \since
 * v.5.5.18
 */
struct common_data_t
	{
		//! Dispatcher's queue.
		dispatcher_queue_t * m_disp_queue;

		//! ID of thread.
		/*!
		 * Receives actual value inside body().
		 */
		so_5::current_thread_id_t m_thread_id;

		//! Actual thread.
		std::thread m_thread;

		//! Waiting object for long wait.
		so_5::disp::mpmc_queue_traits::condition_unique_ptr_t m_condition;

		common_data_t( dispatcher_queue_t & queue )
			:	m_disp_queue( &queue )
			,	m_condition{ queue.allocate_condition() }
			{}
	};

/*!
 * \brief Part of implementation of work thread without activity tracing.
 *
 * \since
 * v.5.5.18
 */
class no_activity_tracking_impl_t : protected common_data_t
	{
	public :
		//! Initializing constructor.
		no_activity_tracking_impl_t(
			dispatcher_queue_t & queue )
			:	common_data_t( queue )
			{}

		template< typename L >
		void
		take_activity_stats( L ) { /* Nothing to do */ }

	protected :
		void
		work_started() {}

		void
		work_finished() {}

		void
		wait_started() {}

		void
		wait_finished() {}
	};

/*!
 * \brief Part of implementation of work thread with activity tracing.
 *
 * \since
 * v.5.5.18
 */
class with_activity_tracking_impl_t : protected common_data_t
	{
		using activity_tracking_traits = so_5::stats::activity_tracking_stuff::traits;

	public :
		//! Initializing constructor.
		with_activity_tracking_impl_t(
			dispatcher_queue_t & queue )
			:	common_data_t( queue )
			{}

		template< typename L >
		void
		take_activity_stats( L lambda )
			{
				so_5::stats::work_thread_activity_stats_t result;

				result.m_working_stats = m_work_activity_collector.take_stats();
				result.m_waiting_stats = m_waiting_stats_collector.take_stats();

				lambda( result );
			}

	protected :
		//! Lock for activity statistics.
		activity_tracking_traits::lock_t m_stats_lock;

		//! A collector for work activity.
		so_5::stats::activity_tracking_stuff::stats_collector_t<
					so_5::stats::activity_tracking_stuff::external_lock<> >
				m_work_activity_collector{ m_stats_lock };

		//! A collector for waiting stats.
		so_5::stats::activity_tracking_stuff::stats_collector_t<
					so_5::stats::activity_tracking_stuff::external_lock<> >
				m_waiting_stats_collector{ m_stats_lock };

		void
		work_started()
			{
				m_work_activity_collector.start();
			}

		void
		work_finished()
			{
				m_work_activity_collector.stop();
			}

		void
		wait_started()
			{
				m_waiting_stats_collector.start();
			}

		void
		wait_finished()
			{
				m_waiting_stats_collector.stop();
			}
	};

//
// work_thread_template_t
//
/*!
 * \brief Implementation of work_thread in form of template class.
 *
 * \tparam Impl no_activity_tracking_impl_t or with_activity_tracking_impl_t.
 *
 * \since
 * v.5.5.18
 */
template< typename Impl >
class work_thread_template_t : public Impl
	{
	public :
		//! Initializing constructor.
		work_thread_template_t( dispatcher_queue_t & queue )
			:	Impl( queue )
			{}

		void
		join()
			{
				this->m_thread.join();
			}

		//! Launch work thread.
		void
		start()
			{
				this->m_thread = std::thread( [this]() { body(); } );
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
		//! Thread body method.
		void
		body()
			{
				this->m_thread_id = so_5::query_current_thread_id();

				agent_queue_t * agent_queue;
				while( nullptr != (agent_queue = this->pop_agent_queue()) )
					{
						// This guard is necessary to ensure that queue
						// will exist until processing of queue finished.
						agent_queue_ref_t agent_queue_guard( agent_queue );

						process_queue( *agent_queue );
					}
			}

		/*!
		 * \since
		 * v.5.5.18
		 *
		 * \brief An attempt of extraction of non-empty agent queue.
		 *
		 * \note This is noexcept method because its logic can't survive
		 * an exception from m_disp_queue->pop.
		 */
		agent_queue_t *
		pop_agent_queue() SO_5_NOEXCEPT
			{
				agent_queue_t * result = nullptr;

				this->wait_started();

				result = this->m_disp_queue->pop( *(this->m_condition) );

				this->wait_finished();

				return result;
			}

		//! Processing of demands from agent queue.
		void
		process_queue( agent_queue_t & queue )
			{
				std::unique_lock< spinlock_t > lock( queue.lock() );

				auto demand = queue.peek_front();
				if( queue.is_there_not_thread_safe_worker() )
					// We can't process any demand until thread unsafe
					// worker is working.
					return;

				auto hint = demand.m_receiver->so_create_execution_hint( demand );

				bool need_schedule = true;
				if( !hint.is_thread_safe() )
				{
					if( queue.is_there_any_worker() )
						// We can't process not thread safe demand until
						// there are some other workers.
						return;
					else
						need_schedule = queue.worker_started(
								agent_queue_t::not_thread_safe_worker );
				}
				else
					// Threa-safe worker can be started.
					need_schedule = queue.worker_started(
							agent_queue_t::thread_safe_worker );

				SO_5_CHECK_INVARIANT( !(need_schedule && queue.empty()), &queue )
				SO_5_CHECK_INVARIANT(
						!need_schedule || hint.is_thread_safe(), &queue );
				SO_5_CHECK_INVARIANT( !need_schedule || queue.active(), &queue );

				// Next few actions must be done on unlocked queue.
				lock.unlock();

				if( need_schedule )
					this->m_disp_queue->schedule( &queue );

				// For activity tracking if it is turned on.
				this->work_started();

				// Processing of event.
				hint.exec( this->m_thread_id );

				this->work_finished();

				// Next actions must be done on locked queue.
				lock.lock();

				need_schedule = queue.worker_finished(
						hint.is_thread_safe() ?
								agent_queue_t::thread_safe_worker :
								agent_queue_t::not_thread_safe_worker );

				SO_5_CHECK_INVARIANT(
						!need_schedule || queue.active(), &queue );

				lock.unlock();

				if( need_schedule )
					this->m_disp_queue->schedule( &queue );
			}
	};

} /* namespace work_thread_details */

//
// work_thread_no_activity_tracking_t
//
/*!
 * \brief Type of work thread without activity tracking.
 *
 * \since
 * v.5.5.18
 */
using work_thread_no_activity_tracking_t =
		work_thread_details::work_thread_template_t<
				work_thread_details::no_activity_tracking_impl_t >;

//
// work_thread_with_activity_tracking_t
//
/*!
 * \brief Type of work thread without activity tracking.
 *
 * \since
 * v.5.5.18
 */
using work_thread_with_activity_tracking_t =
		work_thread_details::work_thread_template_t<
				work_thread_details::with_activity_tracking_impl_t >;

//
// adaptation_t
//
/*!
 * \brief Adaptation of common implementation of thread-pool-like dispatcher
 * to the specific of this thread-pool dispatcher.
 *
 * \since
 * v.5.5.4
 */
struct adaptation_t
	{
		static const char *
		dispatcher_type_name()
			{
				return "atp"; // adv_thread_pool.
			}

		static bool
		is_individual_fifo( const params_t & params )
			{
				return fifo_t::individual == params.query_fifo();
			}

		static void
		wait_for_queue_emptyness( agent_queue_t & /*queue*/ )
			{
				// This type of agent_queue doesn't require waiting for emptyness.
			}
	};

//
// dispatcher_template_t
//
/*!
 * \brief Template for dispatcher.
 *
 * This template depends on work_thread type (with or without activity
 * tracking).
 *
 * \since
 * v.5.5.18
 */
template< typename Work_Thread >
using dispatcher_template_t =
		so_5::disp::thread_pool::common_implementation::dispatcher_t<
				Work_Thread,
				dispatcher_queue_t,
				agent_queue_t,
				params_t,
				adaptation_t >;

} /* namespace impl */

} /* namespace adv_thread_pool */

} /* namespace disp */

} /* namespace so_5 */


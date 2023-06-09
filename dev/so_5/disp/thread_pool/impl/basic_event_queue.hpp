/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief A reusable implementation of agent_queue that can be used by
 * various thread-pool dispatchers.
 *
 * \since v.5.8.0
 */

#pragma once

#include <so_5/disp/reuse/queue_of_queues.hpp>

#include <so_5/event_queue.hpp>
#include <so_5/outliving.hpp>
#include <so_5/spinlocks.hpp>

namespace so_5
{

namespace disp
{

namespace thread_pool
{

namespace impl
{

using spinlock_t = so_5::default_spinlock_t;

//
// basic_event_queue_t
//
class basic_event_queue_t
	:	public event_queue_t
	{
	protected :
		//! Actual demand in event queue.
		struct demand_t : public execution_demand_t
			{
				//! Next item in queue.
				/*!
				 * \note
				 * It's a dynamically allocated object that has to be deallocated
				 * manually during the destruction of the queue.
				 */
				demand_t * m_next;

				demand_t()
					:	m_next( nullptr )
					{}
				demand_t( execution_demand_t && original )
					:	execution_demand_t( std::move( original ) )
					,	m_next( nullptr )
					{}
			};

	public :
		basic_event_queue_t(
			std::size_t max_demands_at_once )
			:	m_max_demands_at_once( max_demands_at_once )
			,	m_tail_demand( &m_head_demand )
			{}

		~basic_event_queue_t() override
			{
				while( m_head_demand.m_next )
					remove_head();
			}

		/*!
		 * \brief Helper method that implements pushing of a new preallocated
		 * demand to the queue.
		 *
		 * It's intended to be used in implementation of push(),
		 * push_evt_start() and push_evt_finish() methods.
		 *
		 * \since v.5.8.0
		 */
		void
		push_preallocated( std::unique_ptr< demand_t > tail_demand ) noexcept
			{
				bool was_empty;

				{
					std::lock_guard< spinlock_t > lock( m_lock );

					was_empty = (nullptr == m_head_demand.m_next);

					m_tail_demand->m_next = tail_demand.release();
					m_tail_demand = m_tail_demand->m_next;

					++m_size;
				}

				// Scheduling of the queue must be done when queue lock
				// is unlocked.
				if( was_empty )
					this->schedule_on_disp_queue();
			}

		//! Push next demand to queue.
		void
		push( execution_demand_t demand ) override
			{
				std::unique_ptr< demand_t > tail_demand{
						new demand_t( std::move( demand ) ) };

				push_preallocated( std::move(tail_demand) );
			}

		//! Push evt_start demand to the queue.
		void
		push_evt_start( execution_demand_t demand ) override
			{
				// Just delegate the work.
				this->push( std::move(demand) );
			}

		//! Push evt_start demand to the queue.
		/*!
		 * \attention
		 * This method is noexcept but it allocates and allocation
		 * can throw. In that case the whole application will be
		 * terminated.
		 */
		void
		push_evt_finish( execution_demand_t demand ) noexcept override
			{
				// Just delegate the work.
				this->push( std::move(demand) );
			}

		//! Get the front demand from queue.
		/*!
		 * \attention This method must be called only on non-empty queue.
		 */
		execution_demand_t &
		front()
			{
				return *(m_head_demand.m_next);
			}

		/*!
		 * \brief Queue emptyness indication.
		 *
		 * \since v.5.5.15.1
		 */
		enum class emptyness_t
			{
				empty,
				not_empty
			};

		/*!
		 * \brief Indication of possibility of continuation of demands processing.
		 *
		 * \since v.5.5.15.1
		 */
		enum class processing_continuation_t
			{
				//! Next demand can be processed.
				enabled,
				disabled
			};

		/*!
		 * \brief A result of erasing of the front demand from queue.
		 *
		 * \since v.5.5.15.1
		 */
		struct pop_result_t
			{
				//! Can demands processing be continued?
				processing_continuation_t m_continuation;
				//! Is event queue empty?
				emptyness_t m_emptyness;
			};

		//! Remove the front demand.
		/*!
		 * \note Return processing_continuation_t::disabled if
		 * \a demands_processed exceeds m_max_demands_at_once or if
		 * event queue is empty.
		 */
		pop_result_t
		pop(
			//! Count of consequently processed demands from that queue.
			std::size_t demands_processed )
			{
				// Actual deletion of old head must be performed
				// when m_lock will be released.
				std::unique_ptr< demand_t > old_head;
				{
					std::lock_guard< spinlock_t > lock( m_lock );

					old_head = remove_head();

					const auto emptyness = m_head_demand.m_next ?
							emptyness_t::not_empty : emptyness_t::empty;

					if( emptyness_t::empty == emptyness )
						m_tail_demand = &m_head_demand;

					return pop_result_t{
							detect_continuation( emptyness, demands_processed ),
							emptyness };
				}
			}

		/*!
		 * \brief Wait while queue becomes empty.
		 *
		 * It is necessary because there is a possibility that
		 * after processing of demand_handler_on_finish cooperation
		 * will be destroyed and agents will be unbound from dispatcher
		 * before the return from demand_handler_on_finish.
		 *
		 * Without waiting for queue emptyness it could lead to
		 * dangling pointer to agent_queue in worker thread.
		 */
		void
		wait_for_emptyness() noexcept
			{
				bool empty = false;
				while( !empty )
					{
						{
							std::lock_guard< spinlock_t > lock( m_lock );
							empty = (nullptr == m_head_demand.m_next);
						}

						if( !empty )
							std::this_thread::yield();
					}
			}

		/*!
		 * \brief Get the current size of the queue.
		 *
		 * \since v.5.5.4
		 */
		[[nodiscard]]
		std::size_t
		size() const noexcept
			{
				return m_size.load( std::memory_order_acquire );
			}

	protected:
		//FIXME: document this
		virtual void
		schedule_on_disp_queue() noexcept = 0;

	private :
		//! Maximum count of demands to be processed consequently.
		const std::size_t m_max_demands_at_once;

		//! Object's lock.
		spinlock_t m_lock;

		//! Head of the demand's queue.
		/*!
		 * Never contains actual demand. Only m_next field is used.
		 */
		demand_t m_head_demand;
		//! Tail of the demand's queue.
		/*!
		 * Must point to m_head_demand if queue is empty or to the very
		 * last queue item otherwise.
		 */
		demand_t * m_tail_demand;

		/*!
		 * \brief Current size of the queue.
		 *
		 * \since v.5.5.4
		 */
		std::atomic< std::size_t > m_size = { 0 };

		//! Helper method for deleting queue's head object.
		inline std::unique_ptr< demand_t >
		remove_head() noexcept
			{
				std::unique_ptr< demand_t > to_be_deleted{ m_head_demand.m_next };
				m_head_demand.m_next = m_head_demand.m_next->m_next;

				--m_size;

				return to_be_deleted;
			}

		//! Can processing be continued?
		inline processing_continuation_t
		detect_continuation(
			emptyness_t emptyness,
			const std::size_t processed )
			{
				return emptyness_t::not_empty == emptyness &&
						processed < m_max_demands_at_once ?
						processing_continuation_t::enabled :
						processing_continuation_t::disabled;
			}
	};

} /* namespace impl */

} /* namespace thread_pool */

} /* namespace disp */

} /* namespace so_5 */


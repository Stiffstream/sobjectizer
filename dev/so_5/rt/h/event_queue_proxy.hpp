/*
	SObjectizer 5.
*/

/*!
	\file
	\since v.5.4.0
	\brief A proxy for event_queue pointer.
*/

#pragma once

#include <so_5/h/atomic_refcounted.hpp>

#include <so_5/rt/h/event_queue.hpp>

#include <mutex>
#include <deque>
#include <memory>

namespace so_5
{

namespace rt
{

/*!
 * \since v.5.4.0
 * \brief A proxy for event_queue pointer.
 */
class event_queue_proxy_t : private atomic_refcounted_t
	{
		friend class intrusive_ptr_t< event_queue_proxy_t >;

	public :
		//! Switch to the specified queue.
		void
		switch_to_actual_queue(
			//! Actual queue.
			event_queue_t & actual_queue,
			//! Agent for which that queue is set.
			agent_t * agent,
			//! Demand handler for so_evt_start event.
			demand_handler_pfn_t start_demand_handler );

		//! Shutdown proxy object.
		/*!
		 * \return last value of event_queue pointer.
		 */
		event_queue_t *
		shutdown();

		//! Enqueue new event to the queue.
		void
		push( execution_demand_t demand );

	private :
		//! Object's lock.
		std::mutex m_lock;

		//! A pointer to the actual event_queue.
		/*!
		 * nullptr value means that event_queue is shut down.
		 */
		event_queue_t * m_actual_queue = nullptr;

		//! Enumeration of possible queue statuses.
		enum class status_t
			{
				//! Queue not started yet.
				not_started,
				//! Queue started and pointed to the actual queue.
				started,
				//! Shutdowned, all new demands must be thrown out.
				stopped
			};
		//! Status of the queue.
		status_t m_status = status_t::not_started;

		//! Type of the temporary queue.
		using temporary_queue_t = std::deque< execution_demand_t >;

		//! Temporary queue.
		/*!
		 * Created only when necessary and removed when proxy is switched
		 * to the actual event_queue.
		 */
		std::unique_ptr< temporary_queue_t > m_tmp_queue;

		//! Move all content of temporary queue to the actual queue.
		void
		move_tmp_queue_to_actual_queue();
	};

/*!
 * \since v.5.4.0
 * \brief Typedef for smart pointer to event_queue_proxy.
 */
typedef intrusive_ptr_t< event_queue_proxy_t >
		event_queue_proxy_ref_t;

} /* namespace rt */

} /* namespace so_5 */


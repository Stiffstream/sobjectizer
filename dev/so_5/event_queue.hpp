/*
	SObjectizer 5.
*/

/*!
	\file
	\since
	v.5.4.0

	\brief An interface of event queue for agent.
*/

#pragma once

#include <so_5/execution_demand.hpp>

namespace so_5
{

/*!
 * \since
 * v.5.4.0
 *
 * \brief An interface of event queue for agent.
 */
class SO_5_TYPE event_queue_t
	{
		// Note: clang-3.9 requires this on Windows platform.
		event_queue_t( const event_queue_t & ) = delete;
		event_queue_t( event_queue_t && ) = delete;
		event_queue_t & operator=( const event_queue_t & ) = delete;
		event_queue_t & operator=( event_queue_t && ) = delete;

	public :
		event_queue_t() = default;
		virtual ~event_queue_t() noexcept = default;

		//! Enqueue new event to the queue.
		/*!
		 * Since v.5.8.0 this method will be used for all demands except
		 * evt_start and evt_finish events. For these events
		 * special methods push_evt_start() and push_evt_finish() are used.
		 *
		 * \note
		 * This method can throw and it's expected.
		 */
		virtual void
		push( execution_demand_t demand ) = 0;

		/*!
		 * \brief Enqueue a demand for evt_start event.
		 *
		 * The first event for an agent is evt_start event. Since v.5.8.0
		 * a demand for the first event is pushed to agent's event_queue
		 * via this method. The ordinal push() method is used for all
		 * other demands except the evt_start and evt_finish demands.
		 *
		 * \note
		 * This method can throw and it's expected.
		 *
		 * \since v.5.8.0
		 */
		virtual void
		push_evt_start( execution_demand_t demand ) = 0;

		/*!
		 * \brief Enqueue a demand for evt_finish event.
		 *
		 * The last event for an agent is evt_finish event. Since v.5.8.0
		 * a demand for the last event is pushed to agent's event_queue
		 * via this method. The ordinal push() method is used for all
		 * other demands except the evt_start and evt_finish demands.
		 *
		 * \note
		 * This method it expected to be noexcept, but if it can't be
		 * implemented for a particular dispatcher then a call to
		 * std::terminate() is appropriate on exception. Because
		 * the normal work can't be continued anyway.
		 *
		 * \since v.5.8.0
		 */
		virtual void
		push_evt_finish( execution_demand_t demand ) noexcept = 0;
	};

} /* namespace so_5 */


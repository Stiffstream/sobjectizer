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
		 * \note
		 * This method can throw and it's expected.
		 */
		virtual void
		push( execution_demand_t demand ) = 0;

		//FIXME: document this!
		/*!
		 * \note
		 * This method can throw and it's expected.
		 *
		 * \since v.5.8.0
		 */
		virtual void
		push_evt_start( execution_demand_t demand ) = 0;

		//FIXME: document this!
		/*!
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


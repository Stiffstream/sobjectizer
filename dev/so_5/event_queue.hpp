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
		virtual void
		push( execution_demand_t demand ) = 0;
	};

} /* namespace so_5 */


/*
	SObjectizer 5.
*/

/*!
	\file
	\since v.5.4.0
	\brief An interface of event queue for agent.
*/

#pragma once

#include <so_5/rt/h/execution_demand.hpp>

namespace so_5
{

/*!
 * \since v.5.4.0
 * \brief An interface of event queue for agent.
 */
class SO_5_TYPE event_queue_t
	{
	public :
		virtual ~event_queue_t();

		//! Enqueue new event to the queue.
		virtual void
		push( execution_demand_t demand ) = 0;
	};

namespace rt
{

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::event_queue_t instead.
 */
using event_queue_t = so_5::event_queue_t;

} /* namespace rt */

} /* namespace so_5 */


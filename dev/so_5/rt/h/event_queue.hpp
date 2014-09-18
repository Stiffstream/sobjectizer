/*
	SObjectizer 5.
*/

/*!
	\file
	\since v.5.4.0
	\brief An interface of event queue for agent.
*/

#if !defined( _SO_5__RT__EVENT_QUEUE_HPP_ )
#define _SO_5__RT__EVENT_QUEUE_HPP_

#include <so_5/rt/h/execution_demand.hpp>

namespace so_5
{

namespace rt
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

} /* namespace rt */

} /* namespace so_5 */

#endif


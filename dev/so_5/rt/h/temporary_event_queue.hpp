/*
	SObjectizer 5.
*/

/*!
	\file
	\since v.5.4.0
	\brief An implementation of event queue for temporary storing of events.
*/

#if !defined( _SO_5__RT__TEMPORARY_EVENT_QUEUE_HPP_ )
#define _SO_5__RT__TEMPORARY_EVENT_QUEUE_HPP_

#include <mutex>
#include <memory>

#include <so_5/h/compiler_features.hpp>

#include <so_5/rt/h/event_queue.hpp>

namespace so_5
{

namespace rt
{

#if defined( SO_5_MSVC )
	#pragma warning(push)
	#pragma warning(disable: 4251)
#endif

/*!
 * \since v.5.4.0
 * \brief An implementation of event queue for temporary storing of events.
 *
 * Object of this type is used by agent before binding to actual event queue.
 */
class SO_5_TYPE temporary_event_queue_t : public event_queue_t
	{
	public :
		//! Constructor.
		temporary_event_queue_t(
			//! A mutex to be used for protecting temporary queue.
			std::mutex & mutex );
		virtual ~temporary_event_queue_t();

		/*!
		 * \name Implementation of event_queue interface.
		 * \{
		 */
		virtual void
		push( execution_demand_t demand );
		/*!
		 * \}
		 */

		//! Changing temporary queue to the actual one.
		/*!
		 * The first event which will be enqueued to the \a actual_queue is
		 * so_evt_start event. This event is not stored in temporary queue.
		 * And agent's pointer and demand handler is necessary to create
		 * that event.
		 */
		void
		switch_to_actual_queue(
			//! Actual queue.
			event_queue_t & actual_queue,
			//! Agent for which that queue is set.
			agent_t * agent,
			//! Demand handler for so_evt_start event.
			demand_handler_pfn_t start_demand_handler );

	private :
		class temporary_queue_t;

		//! Object lock.
		std::mutex & m_mutex;

		//! Actual queue.
		/*!
		 * nullptr means that there is no actual event queue yet and
		 * \a tmp_queue must be used for new events.
		 */
		event_queue_t * m_actual_queue;

		//! Temporary queue.
		/*!
		 * Created only when necessary.
		 */
		std::unique_ptr< temporary_queue_t > m_tmp_queue;
	};

#if defined( SO_5_MSVC )
	#pragma warning(pop)
#endif

} /* namespace rt */

} /* namespace so_5 */

#endif


/*
	SObjectizer 5.
*/

/*!
	\file
	\brief The real class of a dispatcher with the single working thread.
*/

#if !defined( _SO_5__DISP__ONE_THREAD__IMPL__DISP_HPP_ )
#define _SO_5__DISP__ONE_THREAD__IMPL__DISP_HPP_

#include <so_5/rt/h/disp.hpp>
#include <so_5/disp/reuse/work_thread/h/work_thread.hpp>

#include <utility>

namespace so_5
{

namespace disp
{

namespace one_thread
{

namespace impl
{

//
// dispatcher_t
//

/*!
	\brief A dispatcher with the single working thread and an event queue.
*/
class dispatcher_t
	:
		public so_5::rt::dispatcher_t
{
	public:
		dispatcher_t();
		virtual ~dispatcher_t();

		//! \name Implementation of so_5::rt::dispatcher methods.
		//! \{
		virtual void
		start();

		virtual void
		shutdown();

		virtual void
		wait();
		//! \}

		/*!
		 * \since v.5.4.0
		 * \brief Get a binding information for an agent.
		 */
		so_5::rt::event_queue_t *
		get_agent_binding();

	private:
		//! Working thread for the dispatcher.
		so_5::disp::reuse::work_thread::work_thread_t m_work_thread;
};


} /* namespace impl */

} /* namespace one_thread */

} /* namespace disp */

} /* namespace so_5 */

#endif

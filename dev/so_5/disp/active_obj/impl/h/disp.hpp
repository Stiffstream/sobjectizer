/*
	SObjectizer 5.
*/

/*!
	\file
	\brief Real class of the active objects dispatcher.
*/

#if !defined( _SO_5__DISP__ACTIVE_OBJ__IMPL__DISP_HPP_ )
#define _SO_5__DISP__ACTIVE_OBJ__IMPL__DISP_HPP_

#include <map>
#include <mutex>

#include <so_5/rt/h/disp.hpp>
#include <so_5/rt/h/event_queue.hpp>

#include <so_5/disp/reuse/work_thread/h/work_thread.hpp>

namespace so_5 {

namespace rt
{
	class agent_t;

} /* namespace rt */


namespace disp
{

namespace active_obj
{

namespace impl
{

//
// dispatcher_t
//

/*!
	\brief Active objects dispatcher.
*/
class dispatcher_t
	:
		public so_5::rt::dispatcher_t
{
	public:
		dispatcher_t();
		virtual ~dispatcher_t();

		//! \name Implemetation of so_5::rt::dispatcher methods.
		//! \{

		virtual void
		start();

		virtual void
		shutdown();

		virtual void
		wait();

		//! \}

		//! Creates a new thread for the agent specified.
		so_5::rt::event_queue_t *
		create_thread_for_agent( const so_5::rt::agent_t & agent );

		//! Destroys the thread for the agent specified.
		void
		destroy_thread_for_agent( const so_5::rt::agent_t & agent );

	private:
		//! Typedef for mapping from agents to their working threads.
		typedef std::map<
				const so_5::rt::agent_t *,
				so_5::disp::reuse::work_thread::work_thread_shptr_t >
			agent_thread_map_t;

		//! A map from agents to single thread dispatchers.
		agent_thread_map_t m_agent_threads;

		//! Shutdown flag.
		bool m_shutdown_started;

		//! This object lock.
		std::mutex m_lock;
};

} /* namespace impl */

} /* namespace active_obj */

} /* namespace disp */

} /* namespace so_5 */

#endif


/*
	SObjectizer 5.
*/

/*!
	\file
	\brief The real class of the active group dispatcher.
*/

#if !defined( _SO_5__DISP__ACTIVE_GROUP__IMPL__DISP_HPP_ )
#define _SO_5__DISP__ACTIVE_GROUP__IMPL__DISP_HPP_

#include <map>
#include <mutex>

#include <so_5/rt/h/disp.hpp>

#include <so_5/disp/reuse/work_thread/h/work_thread.hpp>

namespace so_5 {

namespace disp {

namespace active_group {

namespace impl {

//
// dispatcher_t
//

/*!
	\brief Active group dispatcher.
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

		//! Launch the dispatcher.
		virtual void
		start();

		//! Send a signal about shutdown to the dispatcher.
		virtual void
		shutdown();

		//! Wait for the full stop of the dispatcher.
		virtual void
		wait();
		//! \}

		/*!
		 * \brief Get the event_queue for the specified active group.
		 *
		 * If name \a group_name is unknown then a new work
		 * thread is started. This thread is marked as it has one
		 * working agent on it.
		 *
		 * If there already is a thread for \a group_name then the
		 * counter of working agents for it is incremented.
		 */
		so_5::rt::event_queue_t *
		query_thread_for_group( const std::string & group_name );

		/*!
		 * \brief Release the thread for the specified active group.
		 *
		 * Method decrements the working agent count for the thread of
		 * \a group_name. If there no more working agents left then
		 * the event_queue and working thread for that group will be
		 * destroyed.
		 */
		void
		release_thread_for_group( const std::string & group_name );

	private:
		//! Auxiliary class for the working agent counting.
		struct thread_with_refcounter_t
		{
			thread_with_refcounter_t(
				so_5::disp::reuse::work_thread::work_thread_shptr_t thread,
				unsigned int user_agent )
				:
					m_thread( std::move( thread ) ),
					m_user_agent( user_agent)
			{}

			so_5::disp::reuse::work_thread::work_thread_shptr_t m_thread;
			unsigned int m_user_agent;
		};

		//! Typedef for mapping from group names to a single thread
		//! dispatcher.
		typedef std::map<
				std::string,
				thread_with_refcounter_t >
			active_group_map_t;

		//! A map of dispatchers for active groups.
		active_group_map_t m_groups;

		//! Shutdown of the indication flag.
		bool m_shutdown_started;

		//! This object lock.
		std::mutex m_lock;
};

} /* namespace impl */

} /* namespace active_group */

} /* namespace disp */

} /* namespace so_5 */

#endif

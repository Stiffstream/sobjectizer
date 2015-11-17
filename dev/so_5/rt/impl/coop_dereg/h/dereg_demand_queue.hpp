/*
	SObjectizer 5.
*/

/*!
	\file
	\brief A cooperation deregistration waiting a queue definition.
*/

#if !defined( _SO_5__RT__IMPL__COOP_DEREG__DEREG_DEMAND_QUEUE_HPP_ )
#define _SO_5__RT__IMPL__COOP_DEREG__DEREG_DEMAND_QUEUE_HPP_

#include <so_5/rt/h/agent_coop.hpp>

#include <mutex>
#include <condition_variable>
#include <deque>

namespace so_5
{

namespace rt
{

namespace impl
{

namespace coop_dereg
{

//
// dereg_demand_queue_t
//

//! A cooperation deregistration waiting a queue.
/*!
 * Thread safe.
 *
 * Also stores shutdown flag.
 */
class dereg_demand_queue_t
{
	public:
		typedef std::deque< coop_t* > dereg_demand_container_t;

		dereg_demand_queue_t();
		~dereg_demand_queue_t();

		//! Put cooperation into the queue.
		void
		push( coop_t * coop );

		//! Get cooperations from the queue.
		/*!
		 * Will block if the queue is empty.
		 *
		 * Returns no cooperations if the shutdown flag is set.
		 */
		void
		pop(
			/*! Demands receiver. */
			dereg_demand_container_t & demands );

		//! Initiate working.
		void
		start_service();

		//! Finish working.
		/*!
		 * Sets up shutdown flag.
		 */
		void
		stop_service();

		/*!
		 * \since v.5.5.12
		 * \brief Get the length of dereg_demands queue.
		 */
		std::size_t
		queue_size();

	private:
		//! Waiting queue.
		dereg_demand_container_t m_demands;

		//! Object lock.
		std::mutex m_lock;

		//! Condition variable for waking up a sleeping thread.
		std::condition_variable m_not_empty;

		//! Working status.
		/*!
		 * Value true means that the queue is in a working state.
		 * Value false serves as the shutdown flag.
		 */
		bool m_in_service = { false };
};

} /* namespace coop_dereg */

} /* namespace impl */

} /* namespace rt */

} /* namespace so_5 */

#endif

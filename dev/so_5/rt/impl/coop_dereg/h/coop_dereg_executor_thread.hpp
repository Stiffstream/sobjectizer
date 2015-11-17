/*
	SObjectizer 5.
*/

/*!
	\file
	\brief A definition for the dedicated thread for the cooperation deregistration.
*/

#if !defined( _SO_5__RT__IMPL__COOP_DEREG__COOP_DEREG_EXECUTOR_THREAD_HPP_ )
#define _SO_5__RT__IMPL__COOP_DEREG__COOP_DEREG_EXECUTOR_THREAD_HPP_

#include <thread>
#include <memory>

#include <so_5/rt/impl/coop_dereg/h/dereg_demand_queue.hpp>

#include <so_5/rt/h/agent_coop.hpp>

namespace so_5
{

namespace rt
{

namespace impl
{

namespace coop_dereg
{

//
// coop_dereg_executor_thread_t
//

//! A dedicated for the cooperation deregistration thread.
/*!
 * This thread is used for doing actions of the cooperation deregistration.
 * To execute these actions a cooperation is put itself to the special
 * deregistration queue. This thread handles this queue.
 */
class coop_dereg_executor_thread_t
{
	public:
		coop_dereg_executor_thread_t();
		~coop_dereg_executor_thread_t();

		//! Launch thread.
		void
		start();

		//! Send a shutdown signal to a thread and wait for full stop.
		void
		finish();

		//! Push cooperation to the waiting queue.
		void
		push_dereg_demand( coop_t * coop );

		/*!
		 * \since v.5.5.12
		 * \brief Get the length of dereg_demands queue.
		 */
		std::size_t
		queue_size();

	protected:
		//! Thread body.
		void
		body();

	private:
		//! Waiting queue.
		dereg_demand_queue_t m_dereg_demand_queue;

		//! Actual thread object.
		std::unique_ptr< std::thread > m_thread;
};

} /* namespace coop_dereg */

} /* namespace impl */

} /* namespace rt */

} /* namespace so_5 */

#endif

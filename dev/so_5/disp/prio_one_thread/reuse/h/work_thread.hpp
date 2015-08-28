/*
	SObjectizer 5.
*/

/*!
 * \since v.5.5.8
 * \file
 * \brief A working thread for dispatcher with one common working
 * thread and support of demands priority.
 */

#pragma once

#include <so_5/h/current_thread_id.hpp>

#include <thread>

namespace so_5 {

namespace disp {

namespace prio_one_thread {

namespace reuse {

//
// work_thread_t
//
/*!
 * \since v.5.5.8
 * \brief A working thread for dispatcher with one common working
 * thread and support of demands priority.
 */
template< typename DEMAND_QUEUE >
class work_thread_t
	{
	public :
		//! Initializing constructor.
		work_thread_t( DEMAND_QUEUE & queue )
			:	m_queue( queue )
			{}

		void
		start()
			{
				m_thread = std::thread( [this]() { body(); } );
			}

		void
		join()
			{
				m_thread.join();
			}

	private :
		//! Demands queue to work for.
		DEMAND_QUEUE & m_queue;

		//! Thread object.
		std::thread m_thread;

		void
		body()
			{
				const auto thread_id = so_5::query_current_thread_id();

				try
					{
						for(;;)
							{
								auto d = m_queue.pop();
								d->call_handler( thread_id );
							}
					}
				catch( const typename DEMAND_QUEUE::shutdown_ex_t & )
					{}
			}
	};

} /* namespace reuse */

} /* namespace prio_one_thread */

} /* namespace disp */

} /* namespace so_5 */


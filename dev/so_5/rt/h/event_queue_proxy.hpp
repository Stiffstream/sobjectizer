/*
	SObjectizer 5.
*/

/*!
	\file
	\since v.5.4.0
	\brief A proxy for event_queue pointer.
*/

#if !defined( _SO_5__RT__EVENT_QUEUE_PROXY_HPP_ )
#define _SO_5__RT__EVENT_QUEUE_PROXY_HPP_

#include <mutex>

#include <so_5/h/spinlocks.hpp>
#include <so_5/h/atomic_refcounted.hpp>

#include <so_5/rt/h/event_queue.hpp>

namespace so_5
{

namespace rt
{

/*!
 * \since v.5.4.0
 * \brief A proxy for event_queue pointer.
 */
class event_queue_proxy_t : private atomic_refcounted_t
	{
		friend class intrusive_ptr_t< event_queue_proxy_t >;

	public :
		event_queue_proxy_t()
			{
				m_queue_ptr = nullptr;
			}

		//! Switch to the specified queue.
		inline void
		switch_to( event_queue_t & queue )
			{
				setup_queue_ptr( &queue );
			}

		//! Shutdown proxy object.
		/*!
		 * \return last value of event_queue pointer.
		 */
		inline event_queue_t *
		shutdown()
			{
				return setup_queue_ptr( nullptr );
			}

		//! Enqueue new event to the queue.
		inline void
		push( execution_demand_t demand )
			{
				read_lock_guard_t< default_rw_spinlock_t > l( m_lock );

				event_queue_t * q = m_queue_ptr.load( std::memory_order_consume );
				if( q )
					q->push( std::move( demand ) );
			}

	private :
		//! Object's lock.
		default_rw_spinlock_t m_lock;

		//! A pointer to the actual event_queue.
		/*!
		 * nullptr value means that event_queue is shut down.
		 */
		std::atomic< event_queue_t * > m_queue_ptr;

		//! Setup a pointer to queue.
		inline event_queue_t *
		setup_queue_ptr( event_queue_t * queue )
			{
				std::lock_guard< default_rw_spinlock_t > l( m_lock );

				auto r = m_queue_ptr.load( std::memory_order_consume );
				m_queue_ptr.store( queue, std::memory_order_release );

				return r;
			}
	};

/*!
 * \since v.5.4.0
 * \brief Typedef for smart pointer to event_queue_proxy.
 */
typedef smart_atomic_reference_t< event_queue_proxy_t >
		event_queue_proxy_ref_t;

} /* namespace rt */

} /* namespace so_5 */

#endif


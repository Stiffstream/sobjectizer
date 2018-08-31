/*
 * SObjectizer 5
 */

/*!
 * \since
 * v.5.5.10
 *
 * \file
 * \brief Various traits for MPSC queues.
 */

#include <so_5/disp/mpsc_queue_traits/h/pub.hpp>

#include <so_5/h/spinlocks.hpp>

#include <so_5/details/h/invoke_noexcept_code.hpp>

#include <mutex>
#include <condition_variable>
#include <iostream>

namespace so_5 {

namespace disp {

namespace mpsc_queue_traits {

namespace impl {

//
// combined_lock_t
//
/*!
 * \since
 * v.5.5.10
 *
 * \brief A special combined lock for queue protection.
 *
 * This lock used spinlocks for efficiency and std::mutex and
 * std::condition_variable for signalization.
 *
 * \attention This lock can be used only for single-consumer queues!
 * It is because there is no way found to implement notify_all on
 * just two int variables (m_waiting and m_signaled). 
 */
class combined_lock_t : public lock_t
	{
	public :
		inline
		combined_lock_t(
			//! Max waiting time for waiting on spinlock before switching to mutex.
			std::chrono::high_resolution_clock::duration waiting_time )
			:	m_waiting_time{ waiting_time }
			,	m_waiting( false )
			,	m_signaled( false )
			{}

		virtual void
		lock() SO_5_NOEXCEPT override
			{
				m_spinlock.lock();
			}

		virtual void
		unlock() SO_5_NOEXCEPT override
			{
				m_spinlock.unlock();
			}

	protected :
		virtual void
		wait_for_notify() SO_5_NOEXCEPT override
			{
				using clock = std::chrono::high_resolution_clock;

				m_waiting = true;
				auto stop_point = clock::now() + m_waiting_time;

				do
					{
						m_spinlock.unlock();

						std::this_thread::yield();

						m_spinlock.lock();

						if( m_signaled )
							{
								m_waiting = false;
								m_signaled = false;
								return;
							}
					}
				while( stop_point > clock::now() );

				// m_lock is locked now.

				// Must use heavy std::mutex and std::condition_variable
				// to allow OS to efficiently use the resources while
				// we are waiting for signal.
				std::unique_lock< std::mutex > mlock( m_mutex );

				m_spinlock.unlock();

				m_condition.wait( mlock, [this]{ return m_signaled; } );

				// At this point m_signaled must be 'true'.

				m_spinlock.lock();

				m_waiting = false;
				m_signaled = false;
			}

		//! Notify one waiting thread if it exists.
		/*!
		 * \attention Must be called only when object is locked.
		 */
		virtual void
		notify_one() SO_5_NOEXCEPT override
			{
				if( m_waiting )
					{
						// There is a waiting thread.
						m_mutex.lock();
						m_signaled = true;
						m_condition.notify_one();
						m_mutex.unlock();
					}
			}

	private :
		const std::chrono::high_resolution_clock::duration m_waiting_time;

		default_spinlock_t m_spinlock;

		std::mutex m_mutex;
		std::condition_variable m_condition;

		bool m_waiting;
		bool m_signaled;
	};

//
// simple_lock_t
//
/*!
 * \since
 * v.5.5.10
 *
 * \brief A very simple lock based on usage of std::mutex and
 * std::condition_variable.
 */
class simple_lock_t : public lock_t
	{
	public :
		virtual void
		lock() SO_5_NOEXCEPT override
			{
				m_mutex.lock();
			}

		virtual void
		unlock() SO_5_NOEXCEPT override
			{
				m_mutex.unlock();
			}

	protected :
		virtual void
		wait_for_notify() SO_5_NOEXCEPT override
			{
				so_5::details::invoke_noexcept_code( [&] {
					// Mutex already locked. We must not try to reacquire it.
					std::unique_lock< std::mutex > mlock{ m_mutex, std::adopt_lock };
					m_condition.wait( mlock, [this]{ return m_signaled; } );
					mlock.release();
				} );

				// At this point m_signaled must be 'true'.
				m_signaled = false;
			}

		virtual void
		notify_one() SO_5_NOEXCEPT override
			{
				m_signaled = true;
				m_condition.notify_one();
			}

	private :
		std::mutex m_mutex;
		std::condition_variable m_condition;

		bool m_signaled = { false };
	};

} /* namespace impl */

//
// combined_lock_factory
//
SO_5_FUNC lock_factory_t
combined_lock_factory(
	std::chrono::high_resolution_clock::duration waiting_time )
	{
		return [waiting_time] {
			return lock_unique_ptr_t{ new impl::combined_lock_t{ waiting_time } };
		};
	}

//
// simple_lock_factory
//
SO_5_FUNC lock_factory_t
simple_lock_factory()
	{
		return [] { return lock_unique_ptr_t{ new impl::simple_lock_t{} }; };
	}

} /* namespace mpsc_queue_traits */

} /* namespace disp */

} /* namespace so_5 */


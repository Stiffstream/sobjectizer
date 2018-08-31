/*
 * SObjectizer 5
 */

/*!
 * \since
 * v.5.5.11
 *
 * \file
 * \brief Various traits for MPMC queues.
 */

#include <so_5/disp/mpmc_queue_traits/h/pub.hpp>

#include <so_5/h/spinlocks.hpp>

#include <mutex>
#include <condition_variable>

namespace so_5 {

namespace disp {

namespace mpmc_queue_traits {

namespace combined_lock
{

using spinlock_t = so_5::default_spinlock_t;

//
// actual_cond_t
//
/*!
 * \since
 * v.5.5.11
 *
 * \brief Impementation of condition object for the case of combined lock.
 */
class actual_cond_t : public condition_t
	{
		//! Spinlock from parent lock object.
		spinlock_t & m_spinlock;
		//! Max waiting time for busy waiting stage.
		const std::chrono::high_resolution_clock::duration m_waiting_time;

		//! An indicator of notification for condition object.
		bool m_signaled = { false };

		//! Personal mutex to be used with condition variable.
		std::mutex m_mutex;
		//! Condition variable for long-time waiting.
		std::condition_variable m_condition;

	public :
		//! Initializing constructor.
		actual_cond_t(
			//! Spinlock from parent lock object.
			spinlock_t & spinlock,
			//! Max waiting time for busy waiting stage.
			std::chrono::high_resolution_clock::duration waiting_time )
			:	m_spinlock( spinlock )
			,	m_waiting_time( std::move(waiting_time) )
			{}

		virtual void
		wait() SO_5_NOEXCEPT override
			{
				using hrc = std::chrono::high_resolution_clock;

				/*
				 * NOTE: spinlock of the parent lock object is already
				 * acquired by the current thread.
				 */
				m_signaled = false;

				//
				// Busy waiting stage.
				//

				// Limitation for busy waiting stage.
				const auto stop_point = hrc::now() + m_waiting_time;

				do
					{
						m_spinlock.unlock();

						std::this_thread::yield();

						m_spinlock.lock();

						if( m_signaled )
							return;
					}
				while( stop_point > hrc::now() );

				// If we are here then busy waiting stage failed (condition
				// is not signaled yet) and we must go to long-time waiting.
				//
				// NOTE: spinlock of the parent lock object is acquired by
				// the current thread.

				//
				// Long-time waiting stage.
				//

				// Personal mutex object must be acquired.
				std::unique_lock< std::mutex > mutex_lock{ m_mutex };
				// Spinlock of the parent lock can be released now.
				m_spinlock.unlock();

				// Wait on condition_variable.
				m_condition.wait( mutex_lock, [this]{ return m_signaled; } );

				// Spinlock must be reacquired to return the parent lock
				// in the state at the call to wait().
				m_spinlock.lock();
			}

		virtual void
		notify() SO_5_NOEXCEPT override
			{
				std::lock_guard< std::mutex > mutex_lock{ m_mutex };

				m_signaled = true;

				m_condition.notify_one();
			}
	};

//
// actual_lock_t
//
/*!
 * \since
 * v.5.5.11
 *
 * \brief Actual implementation of combined lock object.
 */
class actual_lock_t : public lock_t
	{
		//! Common spinlock for locking of producers and consumers.
		spinlock_t m_spinlock;
		//! Max waiting time for busy waiting stage.
		const std::chrono::high_resolution_clock::duration m_waiting_time;

	public :
		//! Initializing constructor.
		actual_lock_t(
			//! Max waiting time for busy waiting stage.
			std::chrono::high_resolution_clock::duration waiting_time )
			:	m_waiting_time{ std::move(waiting_time) }
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

		virtual condition_unique_ptr_t
		allocate_condition() override
			{
				return condition_unique_ptr_t{
					new actual_cond_t{ m_spinlock, m_waiting_time } };
			}
	};

} /* namespace combined_lock */

namespace simple_lock
{

//
// actual_cond_t
//
/*!
 * \since
 * v.5.5.11
 *
 * \brief Actual implementation of condition object for the case
 * of simple locking on mutex and condition_variable.
 */
class actual_cond_t : public condition_t
	{
		//! An indicator of notification for condition object.
		bool m_signaled = { false };

		//! Common mutex from the parent lock.
		std::mutex & m_mutex;
		//! Personal condition_variable object for condition object owner.
		std::condition_variable m_condition;

	public :
		//! Initializing constructor.
		actual_cond_t(
			//! Common mutex from the parent lock.
			std::mutex & mutex )
			:	m_mutex( mutex )
			{}

		virtual void
		wait() SO_5_NOEXCEPT override
			{
				m_signaled = false;

				// Common mutex is already acquired. So we can't reacquire it.
				std::unique_lock< std::mutex > mutex_lock{ m_mutex, std::adopt_lock };
				m_condition.wait( mutex_lock, [this]{ return m_signaled; } );
				// Common mutex must remain acquired. So we disable unique_lock
				// to release mutex in the destructor.
				mutex_lock.release();
			}

		virtual void
		notify() SO_5_NOEXCEPT override
			{
				m_signaled = true;

				m_condition.notify_one();
			}
	};

//
// actual_lock_t
//
/*!
 * \since
 * v.5.5.11
 *
 * \brief Actual implementation of lock object for simple locking
 * on mutex and condition variables.
 */
class actual_lock_t : public lock_t
	{
		//! Common mutex for all producers and consumers.
		std::mutex m_mutex;

	public :
		actual_lock_t()
			{}

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

		virtual condition_unique_ptr_t
		allocate_condition() override
			{
				return condition_unique_ptr_t{ new actual_cond_t{ m_mutex } };
			}
	};

} /* namespace simple_lock */

//
// combined_lock_factory
//
SO_5_FUNC lock_factory_t
combined_lock_factory(
	std::chrono::high_resolution_clock::duration waiting_time )
	{
		return [waiting_time] {
				return lock_unique_ptr_t{ new combined_lock::actual_lock_t{
					std::move(waiting_time) } };
			};
	}

//
// simple_lock_factory
//
SO_5_FUNC lock_factory_t
simple_lock_factory()
	{
		return [] {
				return lock_unique_ptr_t{ new simple_lock::actual_lock_t{} };
			};
	}

} /* namespace mpmc_queue_traits */

} /* namespace disp */

} /* namespace so_5 */



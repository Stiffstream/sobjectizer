/*
 * SObjectizer 5
 */

/*!
 * \since v.5.5.11
 * \file
 * \brief Various traits for MPMC queues.
 */

#pragma once

#include <so_5/h/declspec.hpp>
#include <so_5/h/compiler_features.hpp>

#include <functional>
#include <memory>
#include <chrono>

namespace so_5 {

namespace disp {

namespace mpmc_queue_traits {

//
// condition_t
//
/*!
 * \since v.5.5.11
 * \brief An interface for somethine like condition variable for
 * waiting on MPMC queue lock.
 */
class SO_5_TYPE condition_t
	{
	public :
		condition_t();
		condition_t( const condition_t & ) = delete;
		condition_t( condition_t && ) = delete;
		virtual ~condition_t();

		/*!
		 * \brief Waiting on condition.
		 *
		 * This method is intended to be used by queue's customers for
		 * waiting on queue's pop operation.
		 *
		 * \attention This method will be called when parent lock object
		 * is acquired by the current thread.
		 */
		virtual void
		wait() SO_5_NOEXCEPT = 0;

		/*!
		 * \brief Notification for waiting customer.
		 *
		 * \attention This method will be called when parent lock object
		 * is acquired by the current thread.
		 */
		virtual void
		notify() SO_5_NOEXCEPT = 0;
	};

//
// condition_unique_ptr_t
//
/*!
 * \since v.5.5.11
 * \brief Alias of unique_ptr for condition.
 */
using condition_unique_ptr_t = std::unique_ptr< condition_t >;

//
// lock_t
//
/*!
 * \since v.5.5.11
 * \brief An interface for lock for MPMC queue.
 */
class SO_5_TYPE lock_t
	{
	public :
		lock_t();
		lock_t( const lock_t & ) = delete;
		lock_t( lock_t && ) = delete;
		virtual ~lock_t();

		//! Lock object in exclusive mode.
		virtual void
		lock() SO_5_NOEXCEPT = 0;

		//! Unlock object locked in exclusive mode.
		virtual void
		unlock() SO_5_NOEXCEPT = 0;

		//! Create condition object for another MPMC queue's customer.
		virtual condition_unique_ptr_t
		allocate_condition() = 0;
	};

//
// lock_unique_ptr_t
//
/*!
 * \since v.5.5.11
 * \brief An alias for unique_ptr for lock.
 */
using lock_unique_ptr_t = std::unique_ptr< lock_t >; 

//
// lock_factory_t
//
/*!
 * \since v.5.5.11
 * \brief Type of lock factory.
 */
using lock_factory_t = std::function< lock_unique_ptr_t() >;

//
// default_combined_lock_waiting_time
//
/*!
 * \since v.5.5.11
 * \brief Default timeout used by combined_lock for waiting on spinlock
 * before switching to mutex-based locking scheme.
 */
inline std::chrono::high_resolution_clock::duration
default_combined_lock_waiting_time()
	{
		return std::chrono::milliseconds(1);
	}

/*!
 * \since v.5.5.11
 * \brief Factory for creation of combined queue lock with the specified
 * waiting time.
 *
 * \par Usage example:
	\code
	so_5::launch( []( so_5::rt::environment_t & env ) { ... },
		[]( so_5::rt::environment_params_t & params ) {
			// Add another thread_pool dispatcher with combined_lock for
			// event queue protection.
			using namespace so_5::disp::thread_pool;
			params.add_named_dispatcher(
				"helpers_disp",
				create_disp( disp_params_t{}.tune_queue_params(
					[]( queue_traits::queue_params_t & queue_params ) {
						// Switching to mutex will be after waiting for 500us.
						queue_params.lock_factory( queue_traits::combined_lock_factory(
							std::chrono::microseconds(500) ) );
					} ) ) );
		} );
	\endcode
 */
SO_5_FUNC lock_factory_t
combined_lock_factory(
	//! Max waiting time for waiting on spinlock before switching to mutex.
	std::chrono::high_resolution_clock::duration waiting_time );

//
// combined_lock_factory
//
/*!
 * \since v.5.5.11
 * \brief Factory for creation of combined queue lock with default waiting time.
 *
 * \par Usage example:
	\code
	so_5::launch( []( so_5::rt::environment_t & env ) { ... },
		[]( so_5::rt::environment_params_t & params ) {
			// Add another thread_pool dispatcher with combined_lock for
			// event queue protection.
			using namespace so_5::disp::thread_pool;
			params.add_named_dispatcher(
				"helpers_disp",
				create_disp( disp_params_t{}.tune_queue_params(
					[]( queue_traits::queue_params_t & queue_params ) {
						queue_params.lock_factory( queue_traits::combined_lock_factory() );
					} ) ) );
		} );
	\endcode
 */
inline lock_factory_t
combined_lock_factory()
	{
		return combined_lock_factory(
				default_combined_lock_waiting_time() );
	}

/*!
 * \since v.5.5.11
 * \brief Factory for creation of very simple implementation based on
 * usage of mutex and condition_variable only.
 *
 * \par Usage example:
	\code
	so_5::launch( []( so_5::rt::environment_t & env ) { ... },
		[]( so_5::rt::environment_params_t & params ) {
			// Add another thread_pool dispatcher with simple_lock for
			// event queue protection.
			using namespace so_5::disp::thread_pool;
			params.add_named_dispatcher(
				"helpers_disp",
				create_disp( disp_params_t{}.tune_queue_params(
					[]( queue_traits::queue_params_t & queue_params ) {
						queue_params.lock_factory( queue_traits::simple_lock_factory() );
					} ) ) );
		} );
	\endcode
 */
SO_5_FUNC lock_factory_t
simple_lock_factory();

//
// queue_params_t
//
/*!
 * \since v.5.5.11
 * \brief Container for storing parameters for MPMC queue.
 */
class queue_params_t
	{
	public :
		//! Default constructor.
		queue_params_t()
			:	m_lock_factory{ combined_lock_factory() }
			{}
		//! Copy constructor.
		queue_params_t( const queue_params_t & o )
			:	m_lock_factory{ o.m_lock_factory }
			{}
		//! Move constructor.
		queue_params_t( queue_params_t && o )
			:	m_lock_factory{ std::move(o.m_lock_factory) }
			{}

		friend inline void swap( queue_params_t & a, queue_params_t & b )
			{
				using namespace std;
				swap( a.m_lock_factory, b.m_lock_factory );
			}

		//! Copy operator.
		queue_params_t & operator=( const queue_params_t & o )
			{
				queue_params_t tmp{ o };
				swap( *this, tmp );
				return *this;
			}

		//! Move operator.
		queue_params_t & operator=( queue_params_t && o )
			{
				queue_params_t tmp{ std::move(o) };
				swap( *this, tmp );
				return *this;
			}

		//! Setter for lock factory.
		queue_params_t &
		lock_factory( lock_factory_t factory )
			{
				m_lock_factory = std::move( factory );
				return *this;
			}

		//! Getter for lock factory.
		const lock_factory_t &
		lock_factory() const
			{
				return m_lock_factory;
			}

	private :
		//! Lock factory to be used during queue creation.
		lock_factory_t m_lock_factory;
	};

} /* namespace mpmc_queue_traits */

} /* namespace disp */

} /* namespace so_5 */


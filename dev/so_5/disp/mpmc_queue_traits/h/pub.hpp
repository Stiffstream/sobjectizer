/*
 * SObjectizer 5
 */

/*!
 * \file
 * \brief Various traits for MPMC queues.
 * \since
 * v.5.5.11
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
 * \brief An interface for somethine like condition variable for
 * waiting on MPMC queue lock.
 * \since
 * v.5.5.11
 */
class SO_5_TYPE condition_t
	{
	public :
		condition_t( const condition_t & ) = delete;
		condition_t( condition_t && ) = delete;
		condition_t & operator=( const condition_t & ) = delete;
		condition_t & operator=( condition_t && ) = delete;
		condition_t() = default;
		virtual ~condition_t() SO_5_NOEXCEPT = default;

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
 * \brief Alias of unique_ptr for condition.
 * \since
 * v.5.5.11
 */
using condition_unique_ptr_t = std::unique_ptr< condition_t >;

//
// lock_t
//
/*!
 * \brief An interface for lock for MPMC queue.
 * \since
 * v.5.5.11
 */
class SO_5_TYPE lock_t
	{
	public :
		lock_t( const lock_t & ) = delete;
		lock_t( lock_t && ) = delete;
		lock_t & operator=( const lock_t & ) = delete;
		lock_t & operator=( lock_t && ) = delete;

		lock_t() = default;
		virtual ~lock_t() SO_5_NOEXCEPT = default;

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
 * \brief An alias for unique_ptr for lock.
 * \since
 * v.5.5.11
 */
using lock_unique_ptr_t = std::unique_ptr< lock_t >; 

//
// lock_factory_t
//
/*!
 * \brief Type of lock factory.
 * \since
 * v.5.5.11
 */
using lock_factory_t = std::function< lock_unique_ptr_t() >;

//
// default_combined_lock_waiting_time
//
/*!
 * \brief Default timeout used by combined_lock for waiting on spinlock
 * before switching to mutex-based locking scheme.
 * \since
 * v.5.5.11
 */
inline std::chrono::high_resolution_clock::duration
default_combined_lock_waiting_time()
	{
		return std::chrono::milliseconds(1);
	}

/*!
 * \brief Factory for creation of combined queue lock with the specified
 * waiting time.
 *
 * \par Usage example:
	\code
	so_5::launch( []( so_5::environment_t & env ) { ... },
		[]( so_5::environment_params_t & params ) {
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
 *
 * \since
 * v.5.5.11
 */
SO_5_FUNC lock_factory_t
combined_lock_factory(
	//! Max waiting time for waiting on spinlock before switching to mutex.
	std::chrono::high_resolution_clock::duration waiting_time );

//
// combined_lock_factory
//
/*!
 * \brief Factory for creation of combined queue lock with default waiting time.
 *
 * \par Usage example:
	\code
	so_5::launch( []( so_5::environment_t & env ) { ... },
		[]( so_5::environment_params_t & params ) {
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
 *
 * \since
 * v.5.5.11
 */
inline lock_factory_t
combined_lock_factory()
	{
		return combined_lock_factory(
				default_combined_lock_waiting_time() );
	}

/*!
 * \brief Factory for creation of very simple implementation based on
 * usage of mutex and condition_variable only.
 *
 * \par Usage example:
	\code
	so_5::launch( []( so_5::environment_t & env ) { ... },
		[]( so_5::environment_params_t & params ) {
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
 *
 * \since
 * v.5.5.11
 */
SO_5_FUNC lock_factory_t
simple_lock_factory();

//
// queue_params_t
//
/*!
 * \brief Container for storing parameters for MPMC queue.
 * \since
 * v.5.5.11
 */
class queue_params_t
	{
	public :
		//! Default constructor.
		queue_params_t()
			:	m_lock_factory{}
			,	m_next_thread_wakeup_threshold{ 0 }
			{}
		//! Copy constructor.
		queue_params_t( const queue_params_t & o )
			:	m_lock_factory{ o.m_lock_factory }
			,	m_next_thread_wakeup_threshold{ o.m_next_thread_wakeup_threshold }
			{}
		//! Move constructor.
		queue_params_t( queue_params_t && o )
			:	m_lock_factory{ std::move(o.m_lock_factory) }
			,	m_next_thread_wakeup_threshold{
					std::move(o.m_next_thread_wakeup_threshold) }
			{}

		friend inline void swap( queue_params_t & a, queue_params_t & b )
			{
				std::swap( a.m_lock_factory, b.m_lock_factory );
				std::swap( a.m_next_thread_wakeup_threshold, b.m_next_thread_wakeup_threshold );
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

		/*!
		 * \brief Setter for thread wakeup threshold.
		 *
		 * This parametr tells should the dispatcher wake up some
		 * sleeping thread if there is no more than \a value items in queue.
		 * It could be useful if some dispatcher serves agent with very
		 * fast event handlers. Such agents can handle demands from queue
		 * must faster than dispatcher will wake sleeping worker threads.
		 *
		 * Usage example:
		 * \code
			using namespace so_5;
			using namespace so_5::disp::thread_pool;

			environment_t & env = ...;
			auto disp = create_private_disp(
				env,
				"my-thread-pool",
				disp_params_t{}
					.thread_count( 16 )
					.tune_queue_params(
						[]( queue_traits::queue_params_t & qp ) {
							qp.next_thread_wakeup_threshold( 2 );
						} )
				);
		 * \endcode
		 *
		 * \since
		 * v.5.5.16
		 */
		queue_params_t &
		next_thread_wakeup_threshold( std::size_t value )
			{
				m_next_thread_wakeup_threshold = value;
				return *this;
			}

		/*!
		 * \brief Getter for thread wakeup threshold value.
		 * \since
		 * v.5.5.16
		 */
		std::size_t
		next_thread_wakeup_threshold() const
			{
				return m_next_thread_wakeup_threshold;
			}

	private :
		//! Lock factory to be used during queue creation.
		lock_factory_t m_lock_factory;

		/*!
		 * \brief Threshold for wake up next working thread if there are
		 * non-empty queues.
		 *
		 * \since
		 * v.5.5.16
		 */
		std::size_t m_next_thread_wakeup_threshold;
	};

} /* namespace mpmc_queue_traits */

} /* namespace disp */

} /* namespace so_5 */


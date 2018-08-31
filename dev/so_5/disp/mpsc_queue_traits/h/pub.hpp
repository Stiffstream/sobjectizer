/*
 * SObjectizer 5
 */

/*!
 * \file
 * \brief Various traits for MPSC queues.
 *
 * \since
 * v.5.5.10
 */

#pragma once

#include <so_5/h/declspec.hpp>
#include <so_5/h/compiler_features.hpp>

#include <functional>
#include <memory>
#include <chrono>

namespace so_5 {

namespace disp {

namespace mpsc_queue_traits {

//
// lock_t
//
/*!
 * \brief An interface for lock for MPSC queue.
 *
 * \since
 * v.5.5.10
 */
class SO_5_TYPE lock_t
	{
		friend class unique_lock_t;
		friend class lock_guard_t;

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

	protected :
		//! Waiting for nofication.
		/*!
		 * \attention Must be called only when object is locked!
		 */
		virtual void
		wait_for_notify() SO_5_NOEXCEPT = 0;

		//! Notify one waiting thread if it exists.
		/*!
		 * \attention Must be called only when object is locked.
		 */
		virtual void
		notify_one() SO_5_NOEXCEPT = 0;
	};

//
// lock_unique_ptr_t
//
/*!
 * \brief An alias for unique_ptr for lock.
 *
 * \since
 * v.5.5.10
 */
using lock_unique_ptr_t = std::unique_ptr< lock_t >; 

//
// lock_factory_t
//
/*!
 * \brief Type of lock factory.
 *
 * \since
 * v.5.5.10
 */
using lock_factory_t = std::function< lock_unique_ptr_t() >;

//
// default_combined_lock_waiting_time
//
/*!
 * \brief Default timeout used by combined_lock for waiting on spinlock
 * before switching to mutex-based locking scheme.
 *
 * \since
 * v.5.5.10
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
 * \since
 * v.5.5.10
 *
 * \par Usage example:
	\code
	so_5::launch( []( so_5::environment_t & env ) { ... },
		[]( so_5::environment_params_t & params ) {
			// Add another one_thread dispatcher with combined_lock for
			// event queue protection.
			using namespace so_5::disp::one_thread;
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
 * \brief Factory for creation of combined queue lock with default waiting time.
 *
 * \since
 * v.5.5.10
 *
 * \par Usage example:
	\code
	so_5::launch( []( so_5::environment_t & env ) { ... },
		[]( so_5::environment_params_t & params ) {
			// Add another one_thread dispatcher with combined_lock for
			// event queue protection.
			using namespace so_5::disp::one_thread;
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
		return combined_lock_factory( default_combined_lock_waiting_time() );
	}

/*!
 * \brief Factory for creation of very simple implementation based on
 * usage of mutex and condition_variable only.
 *
 * \since
 * v.5.5.10
 *
 * \par Usage example:
	\code
	so_5::launch( []( so_5::environment_t & env ) { ... },
		[]( so_5::environment_params_t & params ) {
			// Add another one_thread dispatcher with simple_lock for
			// event queue protection.
			using namespace so_5::disp::one_thread;
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
// unique_lock_t
//
/*!
 * \brief An analog of std::unique_lock for MPSC queue lock.
 *
 * \since
 * v.5.5.10
 */
class unique_lock_t
	{
	public :
		inline
		unique_lock_t( lock_t & lock )
			:	m_lock( lock )
			{
				m_lock.lock();
			}

		inline
		~unique_lock_t()
			{
				m_lock.unlock();
			}

		unique_lock_t( const unique_lock_t & ) = delete;
		unique_lock_t( unique_lock_t && ) = delete;

		inline void
		wait_for_notify()
			{
				m_lock.wait_for_notify();
			}

	private :
		lock_t & m_lock;
	};

//
// lock_guard_t
//
/*!
 * \brief An analog of std::lock_guard for MPSC queue lock.
 *
 * \since
 * v.5.4.0
 */
class lock_guard_t
	{
	public :
		inline
		lock_guard_t(
			lock_t & lock )
			:	m_lock( lock )
			{
				m_lock.lock();
			}
		inline
		~lock_guard_t()
			{
				m_lock.unlock();
			}

		lock_guard_t( const lock_guard_t & ) = delete;
		lock_guard_t( lock_guard_t && ) = delete;

		inline void
		notify_one()
			{
				m_lock.notify_one();
			}

	private :
		lock_t & m_lock;
	};

//
// queue_params_t
//
/*!
 * \brief Container for storing parameters for MPSC queue.
 *
 * \since
 * v.5.5.10
 */
class queue_params_t
	{
	public :
		//! Default constructor.
		queue_params_t()
			:	m_lock_factory{}
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

/*!
 * \brief Old alias for queue_params for compatibility.
 * \deprecated Use queue_params_t instead.
 */
using params_t = queue_params_t;

} /* namespace mpsc_queue_traits */

} /* namespace disp */

} /* namespace so_5 */


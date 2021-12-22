/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Interfaces for work_thread and work_thread's factory.
 * \since v.5.7.3
 */

#pragma once

#include <so_5/declspec.hpp>
#include <so_5/fwd.hpp>

#include <functional>
#include <memory>

namespace so_5::disp
{

//
// abstract_work_thread_t
//
/*!
 * \brief An interface for one worker thread.
 *
 * All worker threads used by SObjectizer's dispatchers have to implement
 * that interface.
 *
 * A worker thread is used by SObjectizer that way:
 *
 * - an instance of worker thread is obtained from an appropriate work thread
 *   factory by calling abstract_work_thread_factory_t::acquire();
 * - method abstract_work_thread_t::start() is called for the obtained instance;
 * - some time later method abstract_work_thread_t::join() is called for the
 *   obtained instance;
 * - after the call to join() the obtained instance is returned to the work
 *   thread factory by calling abstract_work_thread_factory_t::release().
 *
 * \since v.5.7.3
 */
class SO_5_TYPE abstract_work_thread_t
	{
	public:
		/*!
		 * \brief Type of functor to be passed to start method.
		 *
		 * abstract_work_thread_t::start() receives a function that
		 * will be executed on the context of a new thread. Type of that
		 * functor is described by this typedef.
		 *
		 * \note
		 * A functor passed to abstract_work_thread_t::start() is not
		 * noexcept. It can throw. However, see the description of
		 * abstract_work_thread_t::start() for more details.
		 */
		using body_func_t = std::function< void() >;

		abstract_work_thread_t();

		abstract_work_thread_t( const abstract_work_thread_t & ) = delete;
		abstract_work_thread_t &
		operator=( const abstract_work_thread_t & ) = delete;

		abstract_work_thread_t( abstract_work_thread_t && ) = delete;
		abstract_work_thread_t &
		operator=( abstract_work_thread_t && ) = delete;

		virtual ~abstract_work_thread_t();

		/*!
		 * \brief Start a new thread and execute specified functor on it.
		 *
		 * It is not specified should a new thread be launched or some existing
		 * thread that is reused. The only demand is that this thread should not
		 * execute some other task except the passed \a thread_body functor.
		 *
		 * The implementation should throw if a new thread can't be started.
		 *
		 * \note
		 * It is allowed that \a thread_body can throw. All exceptions should
		 * be intercepted and skipped. An implementation can use some form
		 * of logging for intercepted exceptions, but it isn't required
		 * (the standard impelementation doesn't log anything).
		 */
		virtual void
		start( body_func_t thread_body ) = 0;

		/*!
		 * \brief Stops the current thread until worker thread completes
		 * execution of thread_body passed to previous call to start.
		 *
		 * SObjectizer guarantees that join() is called only once, and if and
		 * only if the previous call to start() is completed successfully
		 * (without exceptions). The join() method won't be called if there
		 * wasn't a previous call to start() method or if the previous call to
		 * start() method threw.
		 *
		 * \attention
		 * This method is not noexcept in the current version of SObjectizer. So
		 * it can throw. However, SObjectizer has no defense from exceptions
		 * thrown from join(). Moveover, most of the calls to join() are done in
		 * noexcept destructors. It means that if join() throws then it will,
		 * probably, lead to the termination of the whole application. It also
		 * means that noexcept may be added to the requirements of join()
		 * implementation in future versions of SObjectizer.
		 *
		 */
		virtual void
		join() = 0;
	};

//
// abstract_work_thread_factory_t
//
/*!
 * \brief An interface of factory for management of worker threads.
 *
 * A worker thread factory can implement different schemes of thread
 * management, for example:
 *
 * - a new instance of worker thread can be allocated dynamically in acquire()
 *   method and destroyed in release() method;
 * - a pool of preallocated worker thread can be used, method acquire() will
 *   take a thread from that pool and return the thread back to the pool in
 *   release() method.
 *
 * SObjectizer doesn't care about the allocation scheme of worker threads. It
 * demands that a reference returned from acquire() method remains valid until
 * it is passed to release() method.
 *
 * SObjectizer guarantees that a thread is taken from acquire() method will be
 * returned back via release() method.
 *
 * \attention
 * An implementation of thread factory should be thread safe. Calls to
 * acquire() and release() can be performed from different threads.
 * For example, a call to acquire() can be made from thread A, but
 * the corresponding call to release() can be made from thread B.
 *
 * \since v.5.7.3
 */
class SO_5_TYPE abstract_work_thread_factory_t
	{
	public:
		abstract_work_thread_factory_t();

		abstract_work_thread_factory_t(
				const abstract_work_thread_factory_t & ) = delete;
		abstract_work_thread_factory_t &
		operator=( const abstract_work_thread_factory_t & ) = delete;

		abstract_work_thread_factory_t(
				abstract_work_thread_factory_t && ) = delete;
		abstract_work_thread_factory_t &
		operator=( abstract_work_thread_factory_t && ) = delete;

		virtual ~abstract_work_thread_factory_t();

		/*!
		 * \brief Get a new worker thread from factory.
		 *
		 * This method should throw an instance of std::exception (or any derived
		 * from std::exception class) in the case when new worker thread can't
		 * be obtained.
		 *
		 * The reference returned should remains valid until it will be
		 * passed to release() method.
		 */
		[[nodiscard]]
		virtual abstract_work_thread_t &
		acquire(
			//! A reference to SObjectizer Environment in that acquire() is called.
			//! That parameter can be useful in case when a single factory
			//! used for several instances of SObjectizer Environment at the
			//! same time.
			so_5::environment_t & env ) = 0;

		/*!
		 * \brief Return a worker thread back to the factory.
		 *
		 * SObjectizer guarantees that \a thread is a reference obtained
		 * by a previous successful call to acquire().
		 *
		 * SObjectizer guarantees that every successful call to acquire()
		 * is paired with a call to release().
		 */
		virtual void
		release(
			//! A reference to worker thread to be returned.
			//! This reference is obtained by a previous call to acquire().
			abstract_work_thread_t & thread ) noexcept = 0;
	};

//
// abstract_work_thread_factory_shptr_t
//
/*!
 * \brief An alias for shared_ptr to abstract_work_thread_factory.
 *
 * \since v.5.7.3
 */
using abstract_work_thread_factory_shptr_t = std::shared_ptr<
		abstract_work_thread_factory_t >;

//
// work_thread_holder_t
//
/*!
 * \brief An analog of unique_ptr for abstract_work_thread.
 *
 * When an instance of abstract_work_thread is no more needed it has
 * to be returned to an appropriate abstract_work_thread_factory.
 * Helper class work_thread_holder_t simplifies that task, in that
 * sense it's an analog of std::unique_ptr.
 *
 * A fully initialized work_thread_holder_t holds a reference to
 * abstract_work_thread_t instance and shared_ptr to the corresponding
 * abstract_work_thread_factory_t. The instance of abstract_work_thread_t
 * will be returned to abstract_work_thread_factory_t in the destructor
 * of work_thread_holder_t.
 *
 * \note
 * This class doesn't call abstract_work_thread_t::start() nor
 * abstract_work_thread_t::join(). It only calls
 * abstract_work_thread_factory_t::release().
 *
 * \attention
 * It is not thread safe.
 *
 * \since v.5.7.3
 */
class [[nodiscard]] work_thread_holder_t
	{
		abstract_work_thread_t * m_thread{};
		abstract_work_thread_factory_shptr_t m_factory{};

	public:
		friend void
		swap( work_thread_holder_t & a, work_thread_holder_t & b ) noexcept
			{
				using std::swap;

				swap( a.m_thread, b.m_thread );
				swap( a.m_factory, b.m_factory );
			}

		work_thread_holder_t() noexcept = default;

		work_thread_holder_t(
			abstract_work_thread_t & thread,
			abstract_work_thread_factory_shptr_t factory ) noexcept
			:	m_thread{ &thread }
			,	m_factory{ std::move(factory) }
			{}

		work_thread_holder_t( const work_thread_holder_t & ) = delete;
		work_thread_holder_t &
		operator=( const work_thread_holder_t & ) = delete;

		work_thread_holder_t( work_thread_holder_t && o ) noexcept
			:	m_thread{ std::exchange( o.m_thread, nullptr ) }
			,	m_factory{ std::exchange(
					o.m_factory, abstract_work_thread_factory_shptr_t{} )
				}
			{}

		work_thread_holder_t &
		operator=( work_thread_holder_t && o ) noexcept
			{
				work_thread_holder_t tmp{ std::move(o) };
				swap( *this, tmp );

				return *this;
			}

		~work_thread_holder_t() noexcept
			{
				if( m_thread )
					m_factory->release( *m_thread );
			}

		/*!
		 * \retval true if it doesn't hold an actual reference to
		 * abstract_work_thread_t.
		 */
		[[nodiscard]]
		bool
		empty() const noexcept
		{
			return nullptr == m_thread;
		}

		/*!
		 * \retval true if it holds an actual reference to
		 * abstract_work_thread_t (e.g. holder is not empty).
		 */
		[[nodiscard]]
		explicit operator bool() const noexcept
		{
			return !empty();
		}

		/*!
		 * \retval true if it doesn't hold an actual reference to
		 * abstract_work_thread_t.
		 */
		[[nodiscard]]
		bool operator!() const noexcept
		{
			return empty();
		}

		/*!
		 * \brief Unsafe method for getting a reference to holding
		 * abstract_work_thread instance.
		 *
		 * No checks are performed inside that method. Use it at your own risk.
		 *
		 * \attention
		 * Calling of that method for an empty holder is UB.
		 */
		[[nodiscard]]
		abstract_work_thread_t &
		unchecked_get() const noexcept
		{
			return *m_thread;
		}
	};

//
// make_std_work_thread_factory
//
/*!
 * \brief Get a standard SObjectizer's work thread factory that is used by default.
 *
 * Usage example:
 * \code
 * so_5::launch( [](so_5::environment_t & env) {
 * 		...
 * 	},
 * 	[&]( so_5::environment_params_t & params ) {
 * 		params.work_thread_factory( some_condition ?
 * 				my_work_thread_factory() : so_5::disp::make_std_work_thread_factory() );
 * 	} );
 * \endcode
 *
 * \since v.5.7.3
 */
[[nodiscard]]
SO_5_FUNC
abstract_work_thread_factory_shptr_t
make_std_work_thread_factory();

} /* namespace so_5::disp */


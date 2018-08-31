/*
	SObjectizer 5.
*/

/*!
	\file
	\since
	v.5.5.0

	\brief Timers and tools for working with timers.
*/

#pragma once

#include <chrono>
#include <functional>

#include <so_5/h/declspec.hpp>
#include <so_5/h/compiler_features.hpp>

#include <so_5/h/error_logger.hpp>
#include <so_5/h/atomic_refcounted.hpp>

#include <so_5/rt/h/mbox.hpp>
#include <so_5/rt/h/message.hpp>

#include <so_5/h/outliving.hpp>

namespace so_5
{

#if defined( SO_5_MSVC )
	#pragma warning(push)
	#pragma warning(disable: 4251)
#endif

//
// timer_t
//
/*!
 * \since
 * v.5.5.0
 *
 * \brief A base class for timer identificator.
 */
class SO_5_TYPE timer_t
	:	private so_5::atomic_refcounted_t
	{
		friend class intrusive_ptr_t< timer_t >;

	public :
		virtual ~timer_t() SO_5_NOEXCEPT = default;

		//! Is this timer event is active?
		virtual bool
		is_active() const SO_5_NOEXCEPT = 0;

		//! Release the timer event.
		virtual void
		release() SO_5_NOEXCEPT = 0;
	};

//
// timer_id_t
//
/*!
 * \since
 * v.5.5.0
 *
 * \brief An indentificator for the timer.
 */
class SO_5_TYPE timer_id_t
	{
	public :
		//! Default constructor.
		timer_id_t() SO_5_NOEXCEPT = default;
		//! Initializing constructor.
		timer_id_t(
			so_5::intrusive_ptr_t< timer_t > && timer ) SO_5_NOEXCEPT
			:	m_timer( std::move(timer) )
			{}

		//! Swapping.
		void
		swap( timer_id_t & o ) SO_5_NOEXCEPT
			{
				m_timer.swap( o.m_timer );
			}

		//! Is this timer event is active?
		bool
		is_active() const SO_5_NOEXCEPT
			{
				return ( m_timer && m_timer->is_active() );
			}

		//! Release the timer event.
		void
		release() SO_5_NOEXCEPT
			{
				if( m_timer )
					m_timer->release();
			}

	private :
		//! Actual timer.
		so_5::intrusive_ptr_t< timer_t > m_timer;
	};

namespace timer_thread
{
	/*!
	 * \deprecated
	 * \brief Synonym for timer_id.
	 *
	 * Saved for compatibility with previous versions.
	 */
	using timer_id_ref_t = timer_id_t;

} /* namespace timer_thread */

//
// timer_thread_stats_t
//
/*!
 * \since
 * v.5.5.4
 *
 * \brief Statistics for run-time monitoring.
 */
struct timer_thread_stats_t
{
	//! Quantity of single-shot timers.
	std::size_t m_single_shot_count;

	//! Quantity of periodic timers.
	std::size_t m_periodic_count;
};

//
// timer_thread_t
//

//! Timer thread interface.
/*!

	All timer threads for SObjectizer must be derived from this class.

	A real timer may not be implemented as a thread. The name of this class is
	just a consequence of some historic reasons.

	A timer is started by timer_thread_t::start() method. To stop timer the
	timer_thread_t::finish() method is used. The finish() method should block
	caller until all timer resources will be released and all dedicated
	timer threads (if any) are completelly stopped.
*/
class SO_5_TYPE timer_thread_t
	{
		timer_thread_t( const timer_thread_t & ) = delete;
		timer_thread_t &
		operator=( const timer_thread_t & ) = delete;

	public:
		timer_thread_t() = default;
		virtual ~timer_thread_t() = default;

		//! Launch timer.
		virtual void
		start() = 0;

		//! Finish timer and wait for full stop.
		virtual void
		finish() = 0;

		//! Push delayed/periodic message to the timer queue.
		/*!
		 * A timer can be deactivated later by using returned timer_id.
		 */
		virtual timer_id_t
		schedule(
			//! Type of message to be sheduled.
			const std::type_index & type_index,
			//! Mbox for message delivery.
			const mbox_t & mbox,
			//! Message to be sent.
			const message_ref_t & msg,
			//! Pause before first message delivery.
			std::chrono::steady_clock::duration pause,
			//! Period for message repetition.
			//! Zero value means single shot delivery.
			std::chrono::steady_clock::duration period ) = 0;

		//! Push anonymous delayed/periodic message to the timer queue.
		/*!
		 * A timer cannot be deactivated later.
		 */
		virtual void
		schedule_anonymous(
			//! Type of message to be sheduled.
			const std::type_index & type_index,
			//! Mbox for message delivery.
			const mbox_t & mbox,
			//! Message to be sent.
			const message_ref_t & msg,
			//! Pause before first message delivery.
			std::chrono::steady_clock::duration pause,
			//! Period for message repetition.
			//! Zero value means single shot delivery.
			std::chrono::steady_clock::duration period ) = 0;

		/*!
		 * \since
		 * v.5.5.4
		 *
		 * \brief Get statistics for run-time monitoring.
		 */
		virtual timer_thread_stats_t
		query_stats() = 0;
	};

//! Auxiliary typedef for timer_thread autopointer.
typedef std::unique_ptr< timer_thread_t > timer_thread_unique_ptr_t;

//
// timer_thread_factory_t
//
/*!
 * \since
 * v.5.5.0
 *
 * \brief Type of factory for creating timer_thread objects.
 */
using timer_thread_factory_t = std::function<
		timer_thread_unique_ptr_t( error_logger_shptr_t ) >;

/*!
 * \name Tools for creating timer threads.
 * \{
 */
/*!
 * \since
 * v.5.5.0
 *
 * \brief Create timer thread based on timer_wheel mechanism.
 * \note Default parameters will be used for timer thread.
 */
SO_5_FUNC timer_thread_unique_ptr_t
create_timer_wheel_thread(
	//! A logger for handling error messages inside timer_thread.
	error_logger_shptr_t logger );

/*!
 * \since
 * v.5.5.0
 *
 * \brief Create timer thread based on timer_wheel mechanism.
 * \note Parameters must be specified explicitely.
 */
SO_5_FUNC timer_thread_unique_ptr_t
create_timer_wheel_thread(
	//! A logger for handling error messages inside timer_thread.
	error_logger_shptr_t logger,
	//! Size of the wheel.
	unsigned int wheel_size,
	//! A size of one time step for the wheel.
	std::chrono::steady_clock::duration granuality );

/*!
 * \since
 * v.5.5.0
 *
 * \brief Create timer thread based on timer_heap mechanism.
 * \note Default parameters will be used for timer thread.
 */
SO_5_FUNC timer_thread_unique_ptr_t
create_timer_heap_thread(
	//! A logger for handling error messages inside timer_thread.
	error_logger_shptr_t logger );

/*!
 * \since
 * v.5.5.0
 *
 * \brief Create timer thread based on timer_heap mechanism.
 * \note Parameters must be specified explicitely.
 */
SO_5_FUNC timer_thread_unique_ptr_t
create_timer_heap_thread(
	//! A logger for handling error messages inside timer_thread.
	error_logger_shptr_t logger,
	//! Initical capacity of heap array.
	std::size_t initial_heap_capacity );

/*!
 * \since
 * v.5.5.0
 *
 * \brief Create timer thread based on timer_list mechanism.
 */
SO_5_FUNC timer_thread_unique_ptr_t
create_timer_list_thread(
	//! A logger for handling error messages inside timer_thread.
	error_logger_shptr_t logger );
/*!
 * \}
 */

/*!
 * \name Standard timer thread factories.
 * \{
 */
/*!
 * \since
 * v.5.5.0
 *
 * \brief Factory for timer_wheel thread with default parameters.
 */
inline timer_thread_factory_t
timer_wheel_factory()
	{
		// Use this trick because create_timer_wheel_thread is overloaded.
		timer_thread_unique_ptr_t (*f)( error_logger_shptr_t ) =
				create_timer_wheel_thread;
		return f;
	}

/*!
 * \since
 * v.5.5.0
 *
 * \brief Factory for timer_wheel thread with explicitely specified parameters.
 */
inline timer_thread_factory_t
timer_wheel_factory(
	//! Size of the wheel.
	unsigned int wheel_size,
	//! A size of one time step for the wheel.
	std::chrono::steady_clock::duration granularity )
	{
		// Use this trick because create_timer_wheel_thread is overloaded.
		timer_thread_unique_ptr_t (*f)(
						error_logger_shptr_t,
						unsigned int,
						std::chrono::steady_clock::duration ) =
				create_timer_wheel_thread;

		using namespace std::placeholders;

		return std::bind( f, _1, wheel_size, granularity );
	}

/*!
 * \since
 * v.5.5.0
 *
 * \brief Factory for timer_heap thread with default parameters.
 */
inline timer_thread_factory_t
timer_heap_factory()
	{
		// Use this trick because create_timer_heap_thread is overloaded.
		timer_thread_unique_ptr_t (*f)( error_logger_shptr_t ) =
				create_timer_heap_thread;
		return f;
	}

/*!
 * \since
 * v.5.5.0
 *
 * \brief Factory for timer_heap thread with explicitely specified parameters.
 */
inline timer_thread_factory_t
timer_heap_factory(
	//! Initial capacity of heap array.
	std::size_t initial_heap_capacity )
	{
		// Use this trick because create_timer_heap_thread is overloaded.
		timer_thread_unique_ptr_t (*f)( error_logger_shptr_t, std::size_t ) =
				create_timer_heap_thread;

		using namespace std::placeholders;

		return std::bind( f, _1, initial_heap_capacity );
	}

/*!
 * \since
 * v.5.5.0
 *
 * \brief Factory for timer_list thread with default parameters.
 */
inline timer_thread_factory_t
timer_list_factory()
	{
		return &create_timer_list_thread;
	}
/*!
 * \}
 */

//
// timer_manager_t
//

//! Timer manager interface.
/*!
 * Interface for all implementations of timer_managers.
 *
 * Timer managers do not create externals threads and must not use
 * any mutexs/spinlocks inside. All work must be done only on the context
 * of the caller thread.
 *
 * \since
 * v.5.5.19
 */
class SO_5_TYPE timer_manager_t
	{
		timer_manager_t( const timer_manager_t & ) = delete;
		timer_manager_t &
		operator=( const timer_manager_t & ) = delete;

	public:
		/*!
		 * \brief An interface for collector of elapsed timers.
		 *
		 * Handling of elapsed timers in single threaded environments
		 * differs from handling in multi-threaded environments.
		 * When timers are handled by separated timer thread then that
		 * thread can send delayed/periodic messages directly to target
		 * mboxes. But in single-threaded environment this is impossible
		 * if some receiver is bound to the default dispatcher. In that
		 * case a deadlock is possible.
		 *
		 * Because of that timer_managers uses different schemes: main
		 * working thread periodically calls process_expired_timers() method
		 * and collects elapsed timers. Then the main working thread handles
		 * all elapsed timers.
		 *
		 * To collect elapsed timer some container is necessary.
		 * The class elapsed_timers_collector_t describes interface of
		 * that container.
		 */
		class SO_5_TYPE elapsed_timers_collector_t
			{
				elapsed_timers_collector_t( const elapsed_timers_collector_t & ) = delete;
				elapsed_timers_collector_t &
				operator=( const elapsed_timers_collector_t & ) = delete;

			public :
				elapsed_timers_collector_t() SO_5_NOEXCEPT = default;
				virtual ~elapsed_timers_collector_t() SO_5_NOEXCEPT = default;

				//! Accept and store info about elapsed timer.
				virtual void
				accept(
					//! A type of message to be sent.
					std::type_index type_index,
					//! Target mbox for the message.
					mbox_t mbox,
					//! A message to be sent.
					message_ref_t msg ) = 0;
			};

		timer_manager_t() SO_5_NOEXCEPT = default;
		virtual ~timer_manager_t() SO_5_NOEXCEPT = default;

		//! Translation of expired timers into message sends.
		virtual void
		process_expired_timers() = 0;

		//! Calculate time before the nearest timer (if any).
		/*!
		 * Return timeout before the nearest timer or \c default_timeout
		 * if there is no any timer.
		 */
		virtual std::chrono::steady_clock::duration
		timeout_before_nearest_timer(
			//! Default timeout if there is no any timer.
			std::chrono::steady_clock::duration default_timer ) = 0;

		//! Push delayed/periodic message to the timer queue.
		/*!
		 * A timer can be deactivated later by using returned timer_id.
		 */
		virtual timer_id_t
		schedule(
			//! Type of message to be sheduled.
			const std::type_index & type_index,
			//! Mbox for message delivery.
			const mbox_t & mbox,
			//! Message to be sent.
			const message_ref_t & msg,
			//! Pause before first message delivery.
			std::chrono::steady_clock::duration pause,
			//! Period for message repetition.
			//! Zero value means single shot delivery.
			std::chrono::steady_clock::duration period ) = 0;

		//! Push anonymous delayed/periodic message to the timer queue.
		/*!
		 * A timer cannot be deactivated later.
		 */
		virtual void
		schedule_anonymous(
			//! Type of message to be sheduled.
			const std::type_index & type_index,
			//! Mbox for message delivery.
			const mbox_t & mbox,
			//! Message to be sent.
			const message_ref_t & msg,
			//! Pause before first message delivery.
			std::chrono::steady_clock::duration pause,
			//! Period for message repetition.
			//! Zero value means single shot delivery.
			std::chrono::steady_clock::duration period ) = 0;

		/*!
		 * \return true if there is no any pending timers.
		 */
		virtual bool
		empty() = 0;

		/*!
		 * \since
		 * v.5.5.4
		 *
		 * \brief Get statistics for run-time monitoring.
		 */
		virtual timer_thread_stats_t
		query_stats() = 0;
	};

//! Auxiliary typedef for timer_manager autopointer.
/*!
 * \since
 * v.5.5.19
 */
using timer_manager_unique_ptr_t = std::unique_ptr< timer_manager_t >;

//
// timer_manager_factory_t
//
/*!
 * \brief Type of factory for creating timer_manager objects.
 *
 * \since
 * v.5.5.19
 */
using timer_manager_factory_t = std::function<
		timer_manager_unique_ptr_t(
				error_logger_shptr_t,
				outliving_reference_t< timer_manager_t::elapsed_timers_collector_t > ) >;

/*!
 * \name Tools for creating timer managers.
 * \{
 */
/*!
 * \brief Create timer manager based on timer_wheel mechanism.
 * \note Default parameters will be used for timer manager.
 *
 * \since
 * v.5.5.19
 */
SO_5_FUNC timer_manager_unique_ptr_t
create_timer_wheel_manager(
	//! A logger for handling error messages inside timer_manager.
	error_logger_shptr_t logger,
	//! A collector for elapsed timers.
	outliving_reference_t< timer_manager_t::elapsed_timers_collector_t >
		collector );

/*!
 * \since
 * v.5.5.0
 *
 * \brief Create timer manager based on timer_wheel mechanism.
 * \note Parameters must be specified explicitely.
 */
SO_5_FUNC timer_manager_unique_ptr_t
create_timer_wheel_manager(
	//! A logger for handling error messages inside timer_manager.
	error_logger_shptr_t logger,
	//! A collector for elapsed timers.
	outliving_reference_t< timer_manager_t::elapsed_timers_collector_t >
		collector,
	//! Size of the wheel.
	unsigned int wheel_size,
	//! A size of one time step for the wheel.
	std::chrono::steady_clock::duration granuality );

/*!
 * \since
 * v.5.5.0
 *
 * \brief Create timer manager based on timer_heap mechanism.
 * \note Default parameters will be used for timer manager.
 */
SO_5_FUNC timer_manager_unique_ptr_t
create_timer_heap_manager(
	//! A logger for handling error messages inside timer_manager.
	error_logger_shptr_t logger,
	//! A collector for elapsed timers.
	outliving_reference_t< timer_manager_t::elapsed_timers_collector_t >
		collector );

/*!
 * \since
 * v.5.5.0
 *
 * \brief Create timer manager based on timer_heap mechanism.
 * \note Parameters must be specified explicitely.
 */
SO_5_FUNC timer_manager_unique_ptr_t
create_timer_heap_manager(
	//! A logger for handling error messages inside timer_manager.
	error_logger_shptr_t logger,
	//! A collector for elapsed timers.
	outliving_reference_t< timer_manager_t::elapsed_timers_collector_t >
		collector,
	//! Initical capacity of heap array.
	std::size_t initial_heap_capacity );

/*!
 * \since
 * v.5.5.0
 *
 * \brief Create timer thread based on timer_list mechanism.
 */
SO_5_FUNC timer_manager_unique_ptr_t
create_timer_list_manager(
	//! A logger for handling error messages inside timer_manager.
	error_logger_shptr_t logger,
	//! A collector for elapsed timers.
	outliving_reference_t< timer_manager_t::elapsed_timers_collector_t >
		collector );
/*!
 * \}
 */

/*!
 * \name Standard timer manager factories.
 * \{
 */
/*!
 * \since
 * v.5.5.0
 *
 * \brief Factory for timer_wheel manager with default parameters.
 */
inline timer_manager_factory_t
timer_wheel_manager_factory()
	{
		// Use this trick because create_timer_wheel_thread is overloaded.
		timer_manager_unique_ptr_t (*f)(
					error_logger_shptr_t,
					outliving_reference_t<
							timer_manager_t::elapsed_timers_collector_t > ) =
				create_timer_wheel_manager;

		return f;
	}

/*!
 * \since
 * v.5.5.0
 *
 * \brief Factory for timer_wheel manager with explicitely specified parameters.
 */
inline timer_manager_factory_t
timer_wheel_manager_factory(
	//! Size of the wheel.
	unsigned int wheel_size,
	//! A size of one time step for the wheel.
	std::chrono::steady_clock::duration granularity )
	{
		// Use this trick because create_timer_wheel_thread is overloaded.
		timer_manager_unique_ptr_t (*f)(
						error_logger_shptr_t,
						outliving_reference_t<
								timer_manager_t::elapsed_timers_collector_t >,
						unsigned int,
						std::chrono::steady_clock::duration ) =
				create_timer_wheel_manager;

		using namespace std::placeholders;

		return std::bind( f, _1, _2, wheel_size, granularity );
	}

/*!
 * \since
 * v.5.5.0
 *
 * \brief Factory for timer_heap manager with default parameters.
 */
inline timer_manager_factory_t
timer_heap_manager_factory()
	{
		// Use this trick because create_timer_heap_thread is overloaded.
		timer_manager_unique_ptr_t (*f)(
						error_logger_shptr_t,
						outliving_reference_t<
								timer_manager_t::elapsed_timers_collector_t > ) =
				create_timer_heap_manager;
		return f;
	}

/*!
 * \since
 * v.5.5.0
 *
 * \brief Factory for timer_heap manager with explicitely specified parameters.
 */
inline timer_manager_factory_t
timer_heap_manager_factory(
	//! Initial capacity of heap array.
	std::size_t initial_heap_capacity )
	{
		// Use this trick because create_timer_heap_thread is overloaded.
		timer_manager_unique_ptr_t (*f)(
						error_logger_shptr_t,
						outliving_reference_t<
								timer_manager_t::elapsed_timers_collector_t >,
						std::size_t ) =
				create_timer_heap_manager;

		using namespace std::placeholders;

		return std::bind( f, _1, _2, initial_heap_capacity );
	}

/*!
 * \since
 * v.5.5.0
 *
 * \brief Factory for timer_list manager with default parameters.
 */
inline timer_manager_factory_t
timer_list_manager_factory()
	{
		return &create_timer_list_manager;
	}
/*!
 * \}
 */

namespace internal_timer_helpers {

/*!
 * \brief Helper function for timer_thread creation.
 *
 * \since
 * v.5.5.0
 */
inline timer_thread_unique_ptr_t
create_appropriate_timer_thread(
	error_logger_shptr_t error_logger,
	const timer_thread_factory_t & user_factory )
{
	if( user_factory )
		return user_factory( std::move( error_logger ) );
	else
		return create_timer_heap_thread( std::move( error_logger ) );
}

} /* namespace internal_timer_helpers */

#if defined( SO_5_MSVC )
	#pragma warning(pop)
#endif

} /* namespace so_5 */


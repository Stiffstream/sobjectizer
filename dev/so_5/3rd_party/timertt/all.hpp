/*
 * TimerThreadTemplate
 */

/*!
 * \file timertt/all.hpp
 * \brief All project's stuff.
 */

#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <vector>

#if defined(_MSC_VER) && (_MSC_VER < 1900)
	// Under VS++12 the alignment will always be 8 bytes.
	#define TIMERTT_ALIGNAS_WORKAROUND(T) __declspec(align(8))

	// VS++12 doesn't support noexcept keyword.
	#define TIMERTT_NOEXCEPT
#else
	#define TIMERTT_ALIGNAS_WORKAROUND(T) alignas(T)
	#define TIMERTT_NOEXCEPT noexcept
#endif

/*!
 * \brief The current version of timertt.
 *
 * Please note that this macro was added only in version 1.2.1.
 * It means that it is better to check presense of TIMERTT_VERSION and
 * only then try to check its value:
 * \code
 * #include <timertt/all.hpp>
 *
 * #if defined(TIMERTT_VERSION)
 * 	#if TIMERTT_VERSION > 1002002
 * 	#endif
 * #endif
 * \endcode
 *
 * The value of TIMERTT_VERSION has YXXXZZZ format in decimal. For
 * example: 1002001 or 1003014. 'Y' means the major version number
 * (e.g. 1 for 1.2.1), 'XXX' means minor version number
 * (e.g. 002 for 1.2.1) and 'ZZZ' means patch number
 * (e.g. 001 for 1.2.1). It means that version 1.2.1 will be
 * represented as 1002001 and 1.3.14 will be represented as 1003014.
 *
 * \since
 * v.1.2.1
 */
#define TIMERTT_VERSION 1002002u

/*!
 * \brief Top-level project's namespace.
 */
namespace timertt
{

/*!
 * \brief Container for thread safety flags.
 *
 * \since
 * v.1.1.0
 */
struct thread_safety
{
	/*!
	 * \brief Indicator for not-thread-safe implementation.
	 *
	 * \since
	 * v.1.1.0
	 */
	struct unsafe {};

	/*!
	 * \brief Indicator for thread-safe implemetation.
	 *
	 * \since
	 * v.1.1.0
	 */
	struct safe {};
};

namespace details
{

//! Status of timer.
enum class timer_status : unsigned int
{
	//! Timer is deactivated.
	/*! It can be activated or destroyed safely. */
	deactivated,
	//! Timer is activated.
	/*! It can be safely deactivated and destroyed. */
	active,
	//! Timer is in execution list and is waiting for execution.
	/*!
	 * It cannot be deactivated and destroyed right now.
	 * Status of timer can only be changed to wait_for_deactivation.
	 * And actual deactivation will be performed later, after
	 * processing of execution list.
	 */
	wait_for_execution,
	//! Timer must be deactivated after processing of execution list.
	/*!
	 * The only possible switch for the timer is to deactivated status.
	 */
	wait_for_deactivation
};

} /* namespace details */

/*!
 * \brief Container for thread-safety-specific type declarations.
 *
 * \note Will be specialized for every thread-safety case.
 *
 * \tparam Thread_Safety must be thread_safety::unsafe or thread_safety::safe.
 *
 * \since
 * v.1.1.0
 */
template< typename Thread_Safety >
struct threading_traits {};

/*!
 * \brief Specialization for not-thread-safe case.
 *
 * \since
 * v.1.1.0
 */
template<>
struct threading_traits< thread_safety::unsafe >
{
	//! Type for reference counters.
	typedef unsigned int reference_counter_type;

	//! Type for holding timer status inside a timer object.
	typedef details::timer_status status_holder_type;
};

/*!
 * \brief Specialization for thread-safe case.
 *
 * \since
 * v.1.1.0
 */
template<>
struct threading_traits< thread_safety::safe >
{
	//! Type for reference counters.
	typedef std::atomic_uint reference_counter_type;

	//! Type for holding timer status inside a timer object.
	typedef std::atomic< details::timer_status > status_holder_type;
};

//
// timer_object
//

/*!
 * \brief Base type for timer demands.
 *
 * \tparam Thread_Safety Thread-safety indicator. Must be thread_safety::unsafe
 * or thread_safety::safe type.
 */
template< typename Thread_Safety >
struct timer_object
{
	//! Reference counter for the demand.
	typename threading_traits< Thread_Safety >::reference_counter_type m_references;

	//! Deafault constructor.
	inline timer_object()
	{
		m_references = 0;
	}

	inline virtual ~timer_object()
	{}

	//! Increment reference counter for the demand.
	static inline void
	increment_references( timer_object * t )
	{
		++(t->m_references);
	}

	//! Decrement reference counter for the demand and destroy
	//! demand if there is no more references.
	static inline void
	decrement_references( timer_object * t )
	{
		if( 0 == --(t->m_references) )
			delete t;
	}
};

//
// scoped_timer_object_holder
//
/*!
 * \brief A special wrapper to be used to hold an actual timer object which
 * is not allocated dynamically.
 *
 * This class is a part of support for scoped timer objects. Every timer
 * engine will define its own `scoped_timer_object` by using this
 * template class. Something like:
 * \code
 * class some_engine
 * {
 * 	struct timer_type { ... };
 * public :
 * 	using scoped_timer_object = scoped_timer_object_holder<timer_type>;
 * 	...
 * };
 * \endcode
 *
 * \par Some implementation details.
 * Version 1.2.0 doesn't change way of working with actual timer objects.
 * They are still reference countable. It means that when a scoped timer object
 * is passed to engine's `activate` method a reference count will be
 * incremented.  When this object is passed to `deactivate` method then
 * reference counter will be decremented. If reference counter becomes zero
 * then the timer object will be deallocated by calling `delete`.
 * To prevent this scoped_timer_object_holder automatically incremented
 * reference counter by 1 in the constructor. It means that the reference
 * counter will not be zero (in normal scenarios).
 *
 * \note
 * This type is not Copyable nor Moveable.
 *
 * \since
 * v.1.2.0
 */
template< typename Actual_Object >
class scoped_timer_object_holder
{
	Actual_Object m_object;

public :
	scoped_timer_object_holder()
	{
		// Actual object must have yet another reference to prevent its deletion.
		Actual_Object::increment_references( &m_object );
	}

	scoped_timer_object_holder( const scoped_timer_object_holder & ) = delete;
	scoped_timer_object_holder( scoped_timer_object_holder && ) = delete;

	Actual_Object *
	ptr() { return &m_object; }
};

//
// timer_object_holder
//
/*!
 * \brief An intrusive smart pointer to timer demand.
 *
 * \tparam Thread_Safety Thread-safety indicator. Must be thread_safety::unsafe
 * or thread_safety::safe type.
 */
template< typename Thread_Safety >
class timer_object_holder
{
public :
	//! Default constructor.
	/*!
	 * Constructs a null pointer.
	 */
	inline timer_object_holder()
		:	m_timer( nullptr )
	{}
	//! Constructor for a raw pointer.
	inline timer_object_holder( timer_object< Thread_Safety > * t )
		:	m_timer( t )
	{
		take_object();
	}
	//! Copy constructor.
	inline timer_object_holder( const timer_object_holder & o )
		:	m_timer( o.m_timer )
	{
		take_object();
	}
	//! Move constructor.
	inline timer_object_holder( timer_object_holder && o )
		:	m_timer( o.m_timer )
	{
		o.m_timer = nullptr;
	}

	//! Constructor for the case when timer object is a scoped timer.
	template< typename Actual_Object >
	inline timer_object_holder(
		scoped_timer_object_holder<Actual_Object> & scoped )
		:	timer_object_holder( scoped.ptr() )
	{}

	//! Destructor.
	inline ~timer_object_holder()
	{
		dismiss_object();
	}

	//! Copy operator.
	inline timer_object_holder &
	operator=( const timer_object_holder & o )
	{
		timer_object_holder t( o );
		swap( t );
		return *this;
	}

	//! Move operator.
	inline timer_object_holder &
	operator=( timer_object_holder && o )
	{
		timer_object_holder t( std::move( o ) );
		swap( t );
		return *this;
	}

	//! Swap values.
	inline void
	swap( timer_object_holder & o )
	{
		auto t = m_timer;
		m_timer = o.m_timer;
		o.m_timer = t;
	}

	/*!
	 * \brief Drop controlled object.
	 */
	inline void
	reset()
	{
		dismiss_object();
	}

	//! Is this a null pointer?
	/*!
		i.e. whether get() != 0.

		\retval true if *this manages an object. 
		\retval false otherwise.
	*/
	inline operator bool() const 
	{
		return nullptr != m_timer;
	}

	/*!
	 * \name Access to object.
	 * \{
	 */
	inline timer_object< Thread_Safety > *
	get() const
	{
		return m_timer;
	}

	template< class O >
	O * 
	cast_to()
	{
		if( !m_timer )
			throw std::runtime_error( "timer is nullptr" );

		return static_cast< O * >(m_timer);
	}
	/*!
	 * \}
	 */

private :
	//! Timer controlled by a smart pointer.
	timer_object< Thread_Safety > * m_timer;

	//! Increment reference count to object if it's not null.
	inline void
	take_object()
	{
		if( m_timer )
			timer_object< Thread_Safety >::increment_references( m_timer );
	}

	//! Decrement reference count to object and delete it if needed.
	inline void
	dismiss_object()
	{
		if( m_timer )
		{
			timer_object< Thread_Safety >::decrement_references( m_timer );
			m_timer = nullptr;
		}
	}
};


//
// default_error_logger
//

/*!
 * \brief Class of default error logger.
 *
 * This class uses std::cerr as the stream for logging errors.
 */
struct default_error_logger
{
	//! Logs error message to std::cerr.
	inline void
	operator()(
		//! The text of error message.
		const std::string & what )
	{
		std::cerr << what << std::endl;
	}
};

//
// default_actor_exception_handler
//

/*!
 * \brief Class of default handler for exceptions thrown from timer actors.
 *
 * Calls std::abort() to terminate application execution.
 */
struct default_actor_exception_handler
{
	//! Handles exception.
	inline void
	operator()(
		//! An exception from timer actor.
		const std::exception & )
	{
		std::abort();
	}
};

//
// default_timer_action_type
//
/*!
 * \brief Defaulf type of timer action.
 *
 * \since
 * v.1.2.0
 */
typedef std::function< void() > default_timer_action_type;

//
// monotonic_clock
//
/*!
 * \brief Type of clock used by all threads.
 */
typedef std::chrono::steady_clock monotonic_clock;

//
// timer_quantities
//
/*!
 * \brief Information about quantities of various timer types.
 *
 * \since
 * v.1.1.1
 */
struct timer_quantities
{
	//! Quantity of single-shot timers.
	std::size_t m_single_shot_count = { 0 };

	//! Quantity of periodic timers.
	std::size_t m_periodic_count = { 0 };
};

/*!
 * \brief An internal namespace with implementation details.
 */
namespace details
{

//
// buffer_allocated_object
//
/*!
 * \brief A special storage to be used for holding non-default constructible
 * objects which are created by demand.
 *
 * \note
 * In C++17 std::optional can be used instead of this class.
 *
 * \since
 * v.1.2.0
 */
template<typename T>
class buffer_allocated_object
{
	TIMERTT_ALIGNAS_WORKAROUND(T) std::array<char, sizeof(T)> buffer_;
	bool allocated_{ false };

	void destroy_if_allocated()
	{
		if(allocated_)
		{
			get()->~T();
			allocated_ = false;
		}
	}

public :
	using pointer = T*;
	using element_type = T;
	using reference = typename std::add_lvalue_reference<T>::type;

	buffer_allocated_object() TIMERTT_NOEXCEPT = default;
	buffer_allocated_object(const buffer_allocated_object &) = delete;
	buffer_allocated_object(buffer_allocated_object &&) = delete;

	~buffer_allocated_object()
	{
		destroy_if_allocated();
	}

	template<typename... Args>
	void allocate(Args &&...args)
	{
		destroy_if_allocated();
		new(buffer_.data()) T(std::forward<Args>(args)...);
		allocated_ = true;
	}

	void destroy()
	{
		destroy_if_allocated();
	}

	operator bool() const TIMERTT_NOEXCEPT
	{
		return allocated_;
	}

	pointer get() const TIMERTT_NOEXCEPT
	{
		return reinterpret_cast<pointer>(const_cast<char *>(buffer_.data()));
	}

	pointer operator->() const TIMERTT_NOEXCEPT
	{
		return get();
	}

	reference operator*() const TIMERTT_NOEXCEPT
	{
		return *get();
	}
};

//
// timer_action_holder
//
/*!
 * \brief A special storage for holding timer actions.
 *
 * If a timer action is represented by std::function<void()> then
 * a simple std::function can be used for holding this timer action.
 * But if a timer action is represented by some user type then
 * we must use a internal buffer and should construct timer action
 * instance inplace only when it is necessary.
 *
 * \since
 * v.1.2.0
 */
template< typename Action_Type >
class timer_action_holder
{
	buffer_allocated_object<Action_Type> m_action;

public:
	timer_action_holder() = default;
	timer_action_holder( const timer_action_holder & ) = delete;
	timer_action_holder( timer_action_holder && ) = delete;

	void
	assign( Action_Type && action )
	{
		m_action.allocate( std::move(action) );
	}

	void
	exec() const
	{
		(*m_action)();
	}
};

template<>
class timer_action_holder< default_timer_action_type >
{
	default_timer_action_type m_action;

public :
	timer_action_holder() = default;
	timer_action_holder( const timer_action_holder & ) = delete;
	timer_action_holder( timer_action_holder && ) = delete;

	void
	assign( default_timer_action_type && action )
	{
		m_action = std::move(action);
	}

	void
	exec() const
	{
		m_action();
	}
};

//
// timer_kind
//
/*!
 * \brief Type of the timer (single-shot or periodic).
 *
 * \since
 * v.1.1.1
 */
enum class timer_kind
{
	//! Timer is a single-shot timer.
	single_shot,
	//! Timer is a periodic timer.
	periodic
};

//
// engine_common
//

/*!
 * \brief A common part for all timer engines.
 *
 * Will be used by concrete engines for storing instances of
 * Error_Logger and Actor_Exception_Handler.
 *
 * Also defines type \a thread_safety to be used later.
 *
 * \tparam Thread_Safety Thread-safety indicator.
 * Must be timertt::thread_safety::unsafe or timertt::thread_safety::safe.
 *
 * \tparam Timer_Action type of functor to perform an user-defined
 * action when timer expires. This must be Moveable and MoveConstructible
 * type.
 *
 * \tparam Error_Logger type of logger for errors detected during
 * timer handling. Interface for error logger is defined
 * by default_error_logger class.
 *
 * \tparam Actor_Exception_Handler type of handler for dealing with
 * exceptions thrown from timer actors. Interface for exception handler
 * is defined by default_actor_exception_handler.
 *
 * \since
 * v.1.1.0
 */
template<
	typename Thread_Safety,
	typename Timer_Action,
	typename Error_Logger,
	typename Actor_Exception_Handler >
class engine_common
{
public :
	//! Indicator of thread-safety.
	using thread_safety = Thread_Safety;

	//! Alias for Timer_Action.
	using timer_action = Timer_Action;

	//! Initializing constructor.
	engine_common(
		Error_Logger error_logger,
		Actor_Exception_Handler exception_handler )
		:	m_error_logger( error_logger )
		,	m_exception_handler( exception_handler )
	{}

	/*!
	 * \brief Get the quantities of timers of various types.
	 *
	 * \since
	 * v.1.1.1
	 */
	timer_quantities
	get_timer_quantities() const
	{
		return this->m_timer_quantities;
	}

protected :
	//! Error logger.
	Error_Logger m_error_logger;

	//! Exception handler.
	Actor_Exception_Handler m_exception_handler;

	/*!
	 * \brief Quantities of timers of various types.
	 *
	 * \since
	 * v.1.1.1
	 */
	timer_quantities m_timer_quantities;

	/*!
	 * \brief Helper method for increment the count of timers of
	 * the specific type.
	 *
	 * \since
	 * v.1.1.1
	 */
	void
	inc_timer_count( timer_kind kind )
	{
		if( timer_kind::single_shot == kind )
			++m_timer_quantities.m_single_shot_count;
		else
			++m_timer_quantities.m_periodic_count;
	}

	/*!
	 * \brief Helper method for decrement the count of timers of
	 * the specific type.
	 *
	 * \since
	 * v.1.1.1
	 */
	void
	dec_timer_count( timer_kind kind )
	{
		if( timer_kind::single_shot == kind )
			--m_timer_quantities.m_single_shot_count;
		else
			--m_timer_quantities.m_periodic_count;
	}

	/*!
	 * \brief Helper method for reseting quantities of timers to zero.
	 *
	 * \since
	 * v.1.1.1
	 */
	void
	reset_timer_count()
	{
		m_timer_quantities = timer_quantities{};
	}
};

//
// timer_wheel_engine_defaults
//
/*!
 * \brief Container for static method with default values for
 * timer_wheel engine.
 *
 * \since
 * v.1.1.0
 */
struct timer_wheel_engine_defaults
{
	//! Default wheel size.
	inline static unsigned int
	default_wheel_size() { return 1000; }

	//! Default tick duration.
	inline static monotonic_clock::duration
	default_granularity() { return std::chrono::milliseconds( 10 ); }
};

//
// timer_wheel_engine
//

/*!
 * \brief A engine for timer wheel mechanism.
 *
 * This class uses <a href="http://www.cs.columbia.edu/~nahum/w6998/papers/ton97-timing-wheels.pdf">timer_wheel</a>
 * mechanism to work with timers.
 * This mechanism is efficient for working with big amount of timers.
 * But it requires that timer thread is working always, even in case
 * when there is no timers. Another price for timer_wheel is the
 * granularity of timer steps.
 *
 * Timer wheel data structure consists from one fixed size vector
 * (the wheel) and several double-linked list (one list for every wheel
 * slot). 
 *
 * At the start of next time step thread switches to next wheel position
 * and handles timers from this position list. After processing
 * all elapsed single-shot timers will be removed and deactivated, and
 * all elapsed periodic timers will be rescheduled for the new time steps.
 *
 * \note At the beginnig of time step thread detects elapsed timers, then
 * unblocks object mutex and calls timer actors for those timers. It means
 * that actors call call timer thread object. And there won't be frequent
 * mutex locking/unlocking operations for building and processing
 * list of elapsed timers. This allows to process millions of timer actor
 * per second.
 *
 * \tparam Thread_Safety Thread-safety indicator.
 * Must be timertt::thread_safety::unsafe or timertt::thread_safety::safe.
 *
 * \tparam Timer_Action type of functor to perform an user-defined
 * action when timer expires. This must be Moveable and MoveConstructible
 * type.
 *
 * \tparam Error_Logger type of logger for errors detected during
 * timer thread execution. Interface for error logger is defined
 * by default_error_logger class.
 *
 * \tparam Actor_Exception_Handler type of handler for dealing with
 * exceptions thrown from timer actors. Interface for exception handler
 * is defined by default_actor_exception_handler.
 */
template<
	typename Thread_Safety,
	typename Timer_Action,
	typename Error_Logger,
	typename Actor_Exception_Handler >
class timer_wheel_engine
	:	public engine_common<
			Thread_Safety, Timer_Action, Error_Logger, Actor_Exception_Handler >
{
	//! An alias for base class.
	using base_type = engine_common<
			Thread_Safety, Timer_Action, Error_Logger, Actor_Exception_Handler >;

	struct timer_type;

public :
	//! Type with default parameters for this engine.
	using defaults_type = timer_wheel_engine_defaults;

	//! Alias for timer_action type.
	using timer_action = typename base_type::timer_action;	

	//! Alias for scoped timer object.
	using scoped_timer_object =
			scoped_timer_object_holder< timer_type >;

	//! Constructor with all parameters.
	timer_wheel_engine(
		//! Size of the wheel.
		unsigned int wheel_size,
		//! Size of time step for the timer_wheel.
		monotonic_clock::duration granularity,
		//! An error logger for timer thread.
		Error_Logger error_logger,
		//! An actor exception handler for timer thread.
		Actor_Exception_Handler exception_handler )
		:	base_type( error_logger, exception_handler )
		,	m_wheel_size( wheel_size )
		,	m_granularity( granularity )
	{
		m_wheel.resize( wheel_size );

		m_current_tick_border = monotonic_clock::now() + m_granularity;
	}

	//! Destructor.
	~timer_wheel_engine()
	{
		clear_all();
	}

	//! Create timer to be activated later.
	timer_object_holder< Thread_Safety >
	allocate()
	{
		return timer_object_holder< Thread_Safety >( new timer_type() );
	}

	//! Activate timer and schedule it for execution.
	/*!
	 * \return Value \a true is returned only when the first timer is added to
	 * the empty wheel.
	 *
	 * \throw std::exception If timer thread is not started.
	 * \throw std::exception If \a timer is already activated.
	 *
	 * \tparam Duration_1 actual type which represents time duration.
	 * \tparam Duration_2 actual type which represents time duration.
	 */
	template< class Duration_1, class Duration_2 >
	bool
	activate(
		//! Timer to be activated.
		timer_object_holder< Thread_Safety > timer,
		//! Pause for timer execution.
		Duration_1 pause,
		//! Repetition period.
		//! If <tt>Duration_2::zero() == period</tt> then timer will be
		//! single-shot.
		Duration_2 period,
		//! Action for the timer.
		timer_action action )
	{
		auto * wheel_timer = timer.template cast_to< timer_type >();
		ensure_timer_deactivated( wheel_timer );

		wheel_timer->m_action.assign( std::move(action) );

		// Timer must be taken under control.
		timer_object< Thread_Safety >::increment_references( wheel_timer );
		// It is an active timer now.
		wheel_timer->m_status = timer_status::active;

		perform_insertion_info_wheel( wheel_timer, pause, period );

		// If wheel was empty and this is the first timer added
		// the value of timer_count must be exactly 1.
		return 1 == this->m_timer_quantities.m_single_shot_count +
				this->m_timer_quantities.m_periodic_count;
	}

	/*!
	 * \brief Perform an attempt to reschedule a timer.
	 *
	 * Before v.1.2.1 there was just one way to reschedule a timer:
	 * method deactivate() must be called first and then method
	 * activate() must be called for the same timer. This approach is
	 * not fast because in the case of thread-safe engines it requires
	 * two operations on a mutex.
	 *
	 * Since v.1.2.1 there is a reschedule() method which does deactivation
	 * of a timer (if it is active) and then new activation of this timer.
	 * All actions are performed by using just one operation on a mutex.
	 *
	 * \note
	 * This operation can fail if the timer to be rescheduled is in processing.
	 * Because of that it is recommended to use such operation for
	 * timer_managers only. But even with timer_managers this operation
	 * should be used with care.
	 *
	 * \attention
	 * It move operator for a timer_action throws then timer will be
	 * deactivated. The state for a timer_action itself will be unknown.
	 * 
	 * \throw std::exception If timer thread is not started.
	 * \throw std::exception If \a timer is in processing right now.
	 *
	 * \tparam Duration_1 actual type which represents time duration.
	 * \tparam Duration_2 actual type which represents time duration.
	 *
	 * \since
	 * v.1.2.1
	 */
	template< class Duration_1, class Duration_2 >
	bool
	reschedule(
		//! Timer to be rescheduled. Must be in activated or deactivated state.
		timer_object_holder< Thread_Safety > timer,
		//! Pause for timer execution.
		Duration_1 pause,
		//! Repetition period.
		//! If <tt>Duration_2::zero() == period</tt> then timer will be
		//! single-shot.
		Duration_2 period,
		//! Action for the timer.
		timer_action action )
	{
		auto * wheel_timer = timer.template cast_to< timer_type >();
		// If timer is deactivated the usual activation logic can be used.
		if( timer_status::deactivated == wheel_timer->m_status )
			return this->activate(
					std::move(timer), pause, period, std::move(action) );
		else if( timer_status::active != wheel_timer->m_status )
		{
			// Timer which is in processing now can't be reactivated.
			throw std::runtime_error( "timer is in processing now, "
					"it can't be rescheduled" );
		}

		// Timer must be removed from the wheel first.
		this->remove_timer_from_wheel( wheel_timer );
		this->dec_timer_count( wheel_timer->kind() );

		// If this assigment throws then we must deactivate the timer.
		try
		{
			wheel_timer->m_action.assign( std::move(action) );
		}
		catch(...)
		{
			wheel_timer->m_status = timer_status::deactivated;
			timer_object< Thread_Safety >::decrement_references( wheel_timer );
			// Exception must be rethrown;
			throw;
		}

		this->perform_insertion_info_wheel( wheel_timer, pause, period );

		return false;
	}

	//! Deactivate timer and remove it from the wheel.
	void
	deactivate( timer_object_holder< Thread_Safety > timer )
	{
		auto wheel_timer = timer.template cast_to< timer_type >();
		if( timer_status::active == wheel_timer->m_status )
		{
			// This is normal active timer. It can be safely
			// deactivated and destroyed.
			remove_timer_from_wheel( wheel_timer );

			wheel_timer->m_status = timer_status::deactivated;

			// Release timer object.
			this->dec_timer_count( wheel_timer->kind() );
			timer_object< Thread_Safety >::decrement_references( wheel_timer );
		}
		else if( timer_status::wait_for_execution == wheel_timer->m_status )
		{
			// This timer is in execution list right now.
			// We can only changed its status.
			// Final deactivation will be done after execution of
			// timers actions.
			wheel_timer->m_status = timer_status::wait_for_deactivation;
		}
	}

	/*!
	 * \brief Build sublist of elapsed timers and process them all.
	 */
	template< typename Unique_Lock >
	void
	process_expired_timers(
		//! Object's lock.
		Unique_Lock & lock )
	{
		/*
		 * NOTE: It is possible that period between consequtive
		 * calls to process_expired_timers will be longer than
		 * m_granuality (it is possible when engine is used inside
		 * timer_manager). In that case it is necessary to process
		 * several wheel positions at once.
		 */
		const auto now = monotonic_clock::now();
		for(;;)
		{
			if( !m_current_tick_processed )
			{
				process_current_wheel_position( lock );

				// After processing all current demands and rescheduling
				// all periodic demands the current_position must be
				// advanced.
				m_current_position += 1;
				if( m_current_position >= m_wheel_size )
					m_current_position = 0;

				m_current_tick_processed = true;
			}

			if( now >= m_current_tick_border )
			{
				// A switch to next tick is necessary.
				m_current_tick_border += m_granularity;
				m_current_tick_processed = false;
			}
			else
				break;
		}
	}

	/*!
	 * \brief Is empty timer list?
	 */
	bool
	empty() const
	{
		return 0 == this->m_timer_quantities.m_single_shot_count &&
				0 == this->m_timer_quantities.m_periodic_count;
	}

	/*!
	 * \brief Get time point of the next timer.
	 *
	 * \attention Must be called only when \a !empty().
	 */
	monotonic_clock::time_point
	nearest_time_point() const
	{
		if( !m_current_tick_processed )
			return monotonic_clock::now();
		else
			return m_current_tick_border;
	}

	/*!
	 * \brief Deactivate all timers and cleanup internal data structures.
	 */
	void
	clear_all()
	{
		for( auto & item : m_wheel )
		{
			timer_type * timer = item.m_head;
			item = wheel_item();

			while( timer )
			{
				timer_type * t = timer;
				timer = timer->m_next;

				t->m_status = timer_status::deactivated;
				timer_object< Thread_Safety >::decrement_references( t );
			}
		}

		// For the case of timer_engine restart.
		this->reset_timer_count();
		this->m_current_tick_border = monotonic_clock::now() + m_granularity;
		this->m_current_position = 0;
	}

private :
	//! Type of wheel timer.
	struct timer_type : public timer_object< Thread_Safety >
	{
		//! Status of the timer.
		typename threading_traits< Thread_Safety >::status_holder_type m_status;

		//! Position in the wheel.
		unsigned int m_position = 0;
		//! Full rolls of wheel before execution of demand.
		unsigned int m_full_rolls_left = 0;

		//! Period in ticks.
		/*!
		 * Zero means that demand is single shot.
		 */
		unsigned int m_period = 0;

		//! Timer action.
		timer_action_holder< timer_action > m_action;

		//! Previous demand in the list.
		timer_type * m_prev = nullptr;
		//! Next demand in the list.
		timer_type * m_next = nullptr;

		timer_type()
		{
			m_status = timer_status::deactivated;
		}

		/*!
		 * \brief Detect type of the timer (single-shot or periodic).
		 *
		 * \since
		 * v.1.1.1
		 */
		timer_kind
		kind() const
		{
			return !m_period ? timer_kind::single_shot : timer_kind::periodic;
		}
	};

	//! Type of wheel's item.
	struct wheel_item
	{
		//! Head of the demand's list.
		timer_type * m_head = nullptr;
		//! Tail of the demand's list.
		timer_type * m_tail = nullptr;
	};

	/*!
	 * \name Object's attributes.
	 * \{
	 */
	//! Size of the wheel.
	const unsigned int m_wheel_size;

	//! Granularity of one time step.
	const monotonic_clock::duration m_granularity;

	//! Index of the current position in the wheel.
	unsigned int m_current_position = 0;

	//! Right border of the current tick.
	/*!
	 * This is the time point at which new tick must be started.
	 */
	monotonic_clock::time_point m_current_tick_border;

	//! Has the current tick been processed?
	bool m_current_tick_processed = false;

	//! The wheel data.
	std::vector< wheel_item > m_wheel;
	/*!
	 * \}
	 */

	/*!
	 * \brief Hard check for deactivation state of the timer.
	 *
	 * \throw std::runtimer_error if timer is not deactivated.
	 */
	static void
	ensure_timer_deactivated( const timer_type * timer )
	{
		if( timer_status::deactivated != timer->m_status )
			throw std::runtime_error( "timer is not in 'deactivated' state" );
	}

	/*!
	 * \brief Perform insertion of a timer into wheel data structure.
	 *
	 * This method added to remove the duplication of code in
	 * activate() and reschedule() methods.
	 *
	 * \note
	 * This method doesn't change reference count to timer object.
	 *
	 * \since
	 * v.1.2.1
	 */
	template< class Duration_1, class Duration_2 >
	void
	perform_insertion_info_wheel(
		//! Timer to be inserted.
		timer_type * wheel_timer,
		//! Pause for timer execution.
		Duration_1 pause,
		//! Repetition period.
		//! If <tt>Duration_2::zero() == period</tt> then timer will be
		//! single-shot.
		Duration_2 period )
	{
		// Calculate the demand position in the wheel.
		this->set_position_in_the_wheel(
				wheel_timer,
				duration_to_ticks( pause ) );

		// Special calculations for the periodic demand.
		if( monotonic_clock::duration::zero() != period )
			wheel_timer->m_period = duration_to_ticks( period );
		else
			wheel_timer->m_period = 0;

		// Timer now can be reinserted into the wheel.
		this->insert_demand_to_wheel( wheel_timer );

		// Count of timers changed.
		this->inc_timer_count( wheel_timer->kind() );
	}

	/*!
	 * \brief Converion of duration to number of time steps.
	 *
	 * \note This implementation performs rounding up for duration
	 * values. For example if granularity is 10ms and duration is
	 * 15ms then result will be 2 time steps.
	 *
	 * \note Never return 0. If duration is less then granularity (even
	 * after rounding up) the value 1 will be returned. E.g. timer
	 * will be scheduled for the next time step.
	 *
	 * \tparam Duration actual type for duration representation.
	 */
	template< class Duration >
	unsigned int
	duration_to_ticks(
		//! Time duration to be converted in time steps count.
		Duration d ) const
	{
		auto d_units = 
				std::chrono::duration_cast< monotonic_clock::duration >( d )
				.count();
		auto g_units = m_granularity.count();

		unsigned int r = static_cast< unsigned int >(
				/*
				 * Add g_units/2 for rounding up.
				 * For example, if d is 24ms and granularity is 10
				 * it will be (24+5)=29, and result will be 2.
				 * But if d is 25ms then (25+5)=30 and result will be 3.
				 */
				(d_units + g_units/2) / g_units );
		if( !r )
			r = 1;
		return r;
	}

	/*!
	 * \brief Calculate and fill up wheel position for the timer.
	 *
	 * timer_type::m_position and timer_type::m_full_rolls_left
	 * will be set for \a wheel_timer.
	 */
	void
	set_position_in_the_wheel(
		//! Timer to modify.
		timer_type * wheel_timer,
		//! Timeout for the timer is time steps.
		unsigned int pause_in_ticks ) const
	{
		wheel_timer->m_position =
				( m_current_position + pause_in_ticks ) % m_wheel_size;
		wheel_timer->m_full_rolls_left = pause_in_ticks / m_wheel_size;
	}

	/*!
	 * \brief Insert timer to the wheel.
	 *
	 * If there is a non-empty timer list for the timer wheel position
	 * the \a wheel_timer will be added to the end of that list.
	 */
	void
	insert_demand_to_wheel( timer_type * wheel_timer )
	{
		wheel_item & item = m_wheel[ wheel_timer->m_position ];
		if( item.m_head )
		{
			// There is a list of demands for the wheel position.
			// New demand must be added to the end of that list.
			wheel_timer->m_prev = item.m_tail;
			wheel_timer->m_next = nullptr;
			item.m_tail->m_next = wheel_timer;
			item.m_tail = wheel_timer;
		}
		else
		{
			// There is no list of demands for this wheel position yet.
			// New list must be started.
			wheel_timer->m_prev = wheel_timer->m_next = nullptr;
			item.m_head = wheel_timer;
			item.m_tail = wheel_timer;
		}
	}

	/*!
	 * \brief Remove timer from the timer_wheel.
	 */
	void
	remove_timer_from_wheel( timer_type * wheel_timer )
	{
		if( wheel_timer->m_prev )
			wheel_timer->m_prev->m_next = wheel_timer->m_next;
		else
			m_wheel[ wheel_timer->m_position ].m_head = wheel_timer->m_next;

		if( wheel_timer->m_next )
			wheel_timer->m_next->m_prev = wheel_timer->m_prev;
		else
			m_wheel[ wheel_timer->m_position ].m_tail = wheel_timer->m_prev;
	}

	/*!
	 * \brief Detect elapsed timers for the current time step and
	 * process them all.
	 *
	 * Object \a lock will be unlocked and then locked back.
	 */
	template< class Unique_Lock >
	void
	process_current_wheel_position(
		Unique_Lock & lock )
	{
		timer_type * exec_list_head = make_exec_list();

		if( exec_list_head )
		{
			exec_actions( lock, exec_list_head );

			utilize_exec_list( exec_list_head );
		}
	}

	/*!
	 * \brief Make list of elapsed timers to be executed.
	 */
	timer_type *
	make_exec_list()
	{
		timer_type * head = nullptr;
		timer_type * tail = nullptr;

		timer_type * timer = m_wheel[ m_current_position ].m_head;
		while( timer )
		{
			if( timer->m_full_rolls_left )
			{
				timer->m_full_rolls_left -= 1;
				timer = timer->m_next;
			}
			else
			{
				timer_type * t = timer;
				timer = timer->m_next;

				remove_timer_from_wheel( t );
				t->m_status = timer_status::wait_for_execution;

				if( head )
				{
					tail->m_next = t;
					t->m_prev = tail;
					t->m_next = nullptr;
					tail = t;
				}
				else
				{
					head = tail = t;
					t->m_prev = t->m_next = nullptr;
				}
			}
		}

		return head;
	}

	/*!
	 * \brief Execute all active timers from the list.
	 */
	template< class Unique_Lock >
	void
	exec_actions(
		//! Object lock.
		//! This lock will be unlocked before execution of actions
		//! and locked back after.
		Unique_Lock & lock,
		//! Head of execution list.
		//! Cannot be nullptr.
		timer_type * head )
	{
		lock.unlock();

		while( head )
		{
			try
			{
				// Status of timer can be changed. So it must be checked
				// just before execution. If timer is waiting for
				// deregistration it must not be executed.
				if( timer_status::wait_for_execution == head->m_status )
					head->m_action.exec();
			}
			catch( const std::exception & x )
			{
				this->m_exception_handler( x );
			}
			catch( ... )
			{
				std::ostringstream ss;
				ss << __FILE__ << "(" << __LINE__ 
					<< "): an unknown exception from timer action";
				this->m_error_logger( ss.str() );
				std::abort();
			}

			head = head->m_next;
		}

		lock.lock();
	}

	/*!
	 * \brief Process list of elapsed timers after execution of
	 * its actions.
	 *
	 * Active periodic timers will be rescheduled. All other timers
	 * will be deactivated and removed.
	 */
	void
	utilize_exec_list(
		//! Head of execution list.
		//! Cannot be null.
		timer_type * head )
	{
		while( head )
		{
			timer_type * t = head;
			head = head->m_next;

			// Actual periodic timer must be rescheduled.
			if( timer_status::wait_for_execution == t->m_status &&
					t->m_period )
			{
				// Timer is active again.
				t->m_status = timer_status::active;

				set_position_in_the_wheel( t, t->m_period );

				insert_demand_to_wheel( t );
			}
			else
			{
				// Timer must be utilized.
				t->m_status = timer_status::deactivated;
				this->dec_timer_count( t->kind() );
				timer_object< Thread_Safety >::decrement_references( t );
			}
		}
	}
};

//
// timer_list_engine_defaults
//
/*!
 * \brief Container for static method with default values for
 * timer_list engine.
 *
 * \note At this moment timer_list engine doesn't not need any
 * default parameter. Because of that this type is empty.
 * But it must be declared because of definition of
 * timer_list_engine::defaults_type.
 *
 * \since
 * v.1.1.0
 */
struct timer_list_engine_defaults
{
};

//
// timer_list_engine
//

/*!
 * \brief An engine for timer list mechanism.
 *
 * This engine uses double-linked list of timers as timer mechanism.
 * This list is ordered. The head of the list is the timer with the
 * minimum time point.
 *
 * When used with timer thread then the thread sleeps until the first timer in
 * the list elapsed. Then thread build sublist of elapsed timers and process
 * them. Single-shot timers are removed after processing. Periodic timers
 * rescheduled (inserted into appropriate places in the list).
 *
 * \note After building sublist of elapsed timers thread
 * unblocks object mutex and calls timer actors for timers from the sublist.
 * And locks object back right after processing. It means
 * that actors call call timer thread object. And there won't be frequent
 * mutex locking/unlocking operations for building and processing
 * sublist of elapsed timers. This allows to process millions of timer actor
 * per second.
 *
 * \attention This type of timer thread is good for situations
 * where there are many timers with equal pauses and repetition periods.
 * In that cases almost all timers will be added to the end of the
 * list. But if there are many timers with very different pauses then
 * operation of activating and rescheduling of timers will be too
 * expensive. Timer thread based on timer_wheel or timer_heap is
 * more appropriate for that scenario.
 *
 * \tparam Thread_Safety Thread-safety indicator.
 * Must be timertt::thread_safety::unsafe or timertt::thread_safety::safe.
 *
 * \tparam Timer_Action type of functor to perform an user-defined
 * action when timer expires. This must be Moveable and MoveConstructible
 * type.
 *
 * \tparam Error_Logger type of logger for errors detected during
 * timer thread execution. Interface for error logger is defined
 * by default_error_logger class.
 *
 * \tparam Actor_Exception_Handler type of handler for dealing with
 * exceptions thrown from timer actors. Interface for exception handler
 * is defined by default_actor_exception_handler.
 */
template<
	typename Thread_Safety,
	typename Time_Action,
	typename Error_Logger,
	typename Actor_Exception_Handler >
class timer_list_engine
	:	public engine_common<
			Thread_Safety, Time_Action, Error_Logger, Actor_Exception_Handler >
{
	//! An alias for base class.
	using base_type = engine_common<
			Thread_Safety, Time_Action, Error_Logger, Actor_Exception_Handler >;

	// Forward declaration.
	struct timer_type;

public :
	//! Type with default parameters for this engine.
	typedef timer_list_engine_defaults defaults_type;

	//! Alias for timer_action type.
	using timer_action = typename base_type::timer_action;	

	//! Alias for scoped timer object.
	using scoped_timer_object =
			scoped_timer_object_holder< timer_type >;

	//! Default constructor.
	timer_list_engine()
		:	timer_list_engine(
				Error_Logger(),
				Actor_Exception_Handler() )
	{}

	//! Constructor with all parameters.
	timer_list_engine(
		//! An error logger for timer thread.
		Error_Logger error_logger,
		//! An actor exception handler for timer thread.
		Actor_Exception_Handler exception_handler )
		:	base_type( error_logger, exception_handler )
	{
	}

	~timer_list_engine()
	{
		clear_all();
	}

	//! Create timer to be activated later.
	timer_object_holder< Thread_Safety >
	allocate()
	{
		return timer_object_holder< Thread_Safety >( new timer_type() );
	}

	//! Activate timer and schedule it for execution.
	/*!
	 *
	 * \return true if the new timer is the first timer in the list.
	 *
	 * \throw std::exception If timer thread is not started.
	 * \throw std::exception If \a timer is already activated.
	 *
	 * \tparam Duration_1 actual type which represents time duration.
	 * \tparam Duration_2 actual type which represents time duration.
	 */
	template< class Duration_1, class Duration_2 >
	bool
	activate(
		//! Timer to be activated.
		timer_object_holder< Thread_Safety > timer,
		//! Pause for timer execution.
		Duration_1 pause,
		//! Repetition period.
		//! If <tt>Duration_2::zero() == period</tt> then timer will be
		//! single-shot.
		Duration_2 period,
		//! Action for the timer.
		timer_action action )
	{
		auto list_timer = timer.template cast_to< timer_type >();
		ensure_timer_deactivated( list_timer );

		// Timer object must be correctly (re)initialized.
		list_timer->m_action.assign( std::move( action ) );
		list_timer->m_when = monotonic_clock::now() + pause;
		list_timer->m_period = std::chrono::duration_cast<
				monotonic_clock::duration >( period );

		// Timer must be taken under control.
		timer_object< Thread_Safety >::increment_references( list_timer );
		// It is an active timer now.
		list_timer->m_status = timer_status::active;

		insert_timer_to_list( list_timer );
		// Count of timers in the list changed.
		this->inc_timer_count( list_timer->kind() );

		return list_timer == m_head;
	}

	/*!
	 * \brief Perform an attempt to reschedule a timer.
	 *
	 * Before v.1.2.1 there was just one way to reschedule a timer:
	 * method deactivate() must be called first and then method
	 * activate() must be called for the same timer. This approach is
	 * not fast because in the case of thread-safe engines it requires
	 * two operations on a mutex.
	 *
	 * Since v.1.2.1 there is a reschedule() method which does deactivation
	 * of a timer (if it is active) and then new activation of this timer.
	 * All actions are performed by using just one operation on a mutex.
	 *
	 * \note
	 * This operation can fail if the timer to be rescheduled is in processing.
	 * Because of that it is recommended to use such operation for
	 * timer_managers only. But even with timer_managers this operation
	 * should be used with care.
	 *
	 * \attention
	 * It move operator for a timer_action throws then timer will be
	 * deactivated. The state for a timer_action itself will be unknown.
	 * 
	 * \throw std::exception If timer thread is not started.
	 * \throw std::exception If \a timer is in processing right now.
	 *
	 * \tparam Duration_1 actual type which represents time duration.
	 * \tparam Duration_2 actual type which represents time duration.
	 *
	 * \since
	 * v.1.2.1
	 */
	template< class Duration_1, class Duration_2 >
	bool
	reschedule(
		//! Timer to be rescheduled.
		timer_object_holder< Thread_Safety > timer,
		//! Pause for timer execution.
		Duration_1 pause,
		//! Repetition period.
		//! If <tt>Duration_2::zero() == period</tt> then timer will be
		//! single-shot.
		Duration_2 period,
		//! Action for the timer.
		timer_action action )
	{
		auto list_timer = timer.template cast_to< timer_type >();
		// If timer is deactivated the usual activation logic can be used.
		if( timer_status::deactivated == list_timer->m_status )
			return this->activate(
					std::move(timer), pause, period, std::move(action) );
		else if( timer_status::active != list_timer->m_status )
		{
			// Timer which is in processing now can't be reactivated.
			throw std::runtime_error( "timer is in processing now, "
					"it can't be rescheduled" );
		}

		// Timer must be removed from the list first.
		this->remove_timer_from_list( list_timer );
		this->dec_timer_count( list_timer->kind() );

		// Timer object must be correctly (re)initialized.
		// If this assigment throws then we must deactivate the timer.
		try
		{
			list_timer->m_action.assign( std::move(action) );
		}
		catch(...)
		{
			list_timer->m_status = timer_status::deactivated;
			timer_object< Thread_Safety >::decrement_references( list_timer );
			// Exception must be rethrown;
			throw;
		}
		list_timer->m_when = monotonic_clock::now() + pause;
		list_timer->m_period = std::chrono::duration_cast<
				monotonic_clock::duration >( period );

		// Updated timer must be placed into the list.
		this->insert_timer_to_list( list_timer );
		// Count of timers in the list changed.
		this->inc_timer_count( list_timer->kind() );

		return list_timer == m_head;
	}

	//! Deactivate timer and remove it from the list.
	void
	deactivate(
		//! Timer to be deactivated.
		timer_object_holder< Thread_Safety > timer )
	{
		auto list_timer = timer.template cast_to< timer_type >();
		if( timer_status::active == list_timer->m_status )
		{
			// This is normal active timer. It can be safely
			// deactivated and destroyed.
			remove_timer_from_list( list_timer );
			// Count of timers in the list changed.
			this->dec_timer_count( list_timer->kind() );

			list_timer->m_status = timer_status::deactivated;

			// Release timer object.
			timer_object< Thread_Safety >::decrement_references( list_timer );
		}
		else if( timer_status::wait_for_execution == list_timer->m_status )
		{
			// This timer is in execution list right now.
			// We can only change its status.
			// Final deactivation will be done after execution of
			// timers actions.
			list_timer->m_status = timer_status::wait_for_deactivation;
		}
	}

	/*!
	 * \brief Build sublist of elapsed timers and process them all.
	 *
	 * Object is unlocked and then locked back.
	 */
	template< typename Unique_Lock >
	void
	process_expired_timers(
		//! Object's lock.
		Unique_Lock & lock )
	{
		timer_type * exec_list_head = make_exec_list();

		if( exec_list_head )
		{
			exec_actions( lock, exec_list_head );

			utilize_exec_list( exec_list_head );
		}
	}

	/*!
	 * \brief Is empty timer list?
	 *
	 * \return true if there is no more timers.
	 */
	bool
	empty() const
	{
		return nullptr == m_head;
	}

	/*!
	 * \brief Get time point of the next timer.
	 *
	 * \attention Must be called only when \a !empty().
	 */
	monotonic_clock::time_point
	nearest_time_point() const
	{
		return m_head->m_when;
	}

	/*!
	 * \brief Deactivate all timers and cleanup internal data structures.
	 */
	void
	clear_all()
	{
		while( m_head )
		{
			auto t = m_head;
			m_head = m_head->m_next;

			t->m_status = timer_status::deactivated;
			timer_object< Thread_Safety >::decrement_references( t );
		}

		// There are no more timers in the list.
		this->reset_timer_count();
		m_tail = nullptr;
	}

private :
	//! Type of list timer.
	struct timer_type : public timer_object< Thread_Safety >
	{
		//! Status of the timer.
		typename threading_traits< Thread_Safety >::status_holder_type m_status;

		//! Time of execution for this timer.
		monotonic_clock::time_point m_when;

		//! Period in ticks.
		/*!
		 * Zero means that demand is single shot.
		 */
		monotonic_clock::duration m_period;

		//! Timer action.
		timer_action_holder< timer_action > m_action;

		//! Previous demand in the list.
		timer_type * m_prev = nullptr;
		//! Next demand in the list.
		timer_type * m_next = nullptr;

		timer_type()
		{
			m_status = timer_status::deactivated;
		}

		/*!
		 * \brief Detect type of timer (single-shot or periodic).
		 *
		 * \since
		 * v.1.1.1
		 */
		timer_kind
		kind() const
		{
			return monotonic_clock::duration::zero() == m_period ?
					timer_kind::single_shot : timer_kind::periodic;
		}
	};

	/*!
	 * \name Object's attributes.
	 * \{
	 */
	//! Head of the list of timers.
	timer_type * m_head = nullptr;

	//! Tail of the list of timers.
	timer_type * m_tail = nullptr;
	/*!
	 * \}
	 */

	/*!
	 * \brief Hard check for deactivation state of the timer.
	 *
	 * \throw std::runtimer_error if timer is not deactivated.
	 */
	static void
	ensure_timer_deactivated( const timer_type * timer )
	{
		if( timer_status::deactivated != timer->m_status )
			throw std::runtime_error( "timer is not in 'deactivated' state" );
	}

	//! Insert timer to the list.
	/*!
	 * Insertion starts from the tail of the list. And if \a timer
	 * has lower timer_type::m_whan value then the last list item
	 * there is an loop of searching appropriate place by going to
	 * the head of the list.
	 *
	 * Doesn't increment reference count for \a timer.
	 */
	void
	insert_timer_to_list(
		//! Timer to be inserted.
		timer_type * timer )
	{
		timer_type * point = m_tail;
		while( point )
		{
			if( point->m_when > timer->m_when )
				point = point->m_prev;
			else
			{
				// This is a point to insertion (new timer must be
				// next to 'point' item).
				timer->m_next = point->m_next;

				if( point->m_next )
					point->m_next->m_prev = timer;
				point->m_next = timer;

				timer->m_prev = point;

				if( point == m_tail )
					// timer must become a new tail for the list.
					m_tail = timer;

				return;
			}
		}

		// Timer must go to the head of the list.
		timer->m_prev = nullptr;
		timer->m_next = m_head;
		if( m_head )
			m_head->m_prev = timer;
		m_head = timer;

		if( !m_tail )
			// List was empty. So there must be new tail of the list.
			m_tail = timer;
	}

	//! Remove the timer from the list.
	/*!
	 * Doesn't decrement reference count for \a timer.
	 */
	void
	remove_timer_from_list(
		timer_type * timer )
	{
		if( timer->m_prev )
			timer->m_prev->m_next = timer->m_next;
		else
			m_head = timer->m_next;

		if( timer->m_next )
			timer->m_next->m_prev = timer->m_prev;
		else
			m_tail = timer->m_prev;
	}

	/*!
	 * \brief Build sublist of elapsed timers.
	 *
	 * All timers in the sublist receive timer_status::wait_for_execution
	 * status.
	 */
	timer_type *
	make_exec_list()
	{
		// If there is no timer return empty list immidiately.
		if( !m_head )
			return nullptr;

		auto tail = m_head;

		const auto now = monotonic_clock::now();

		// Search the first not-elapsed-yet timer.
		while( tail && now >= tail->m_when )
		{
			tail->m_status = timer_status::wait_for_execution;
			tail = tail->m_next;
		}

		if( tail == m_head )
			// There is no elapsed timers.
			return nullptr;

		auto exec_list_head = m_head;
		if( tail )
		{
			// This item must be the new head of the list.
			m_head = tail;
			tail->m_prev->m_next = nullptr;
			tail->m_prev = nullptr;
		}
		else
		{
			// Whole timer list is the execution list.
			m_head = m_tail = nullptr;
		}

		return exec_list_head;
	}

	/*!
	 * \brief Execute all active timers in the sublist.
	 *
	 * Object is unlocked and locked back after sublist processing.
	 */
	template< class Unique_Lock >
	void
	exec_actions(
		//! Object lock.
		//! This lock will be unlocked before execution of actions
		//! and locked back after.
		Unique_Lock & lock,
		//! Head of execution list.
		//! Cannot be nullptr.
		timer_type * head )
	{
		lock.unlock();

		while( head )
		{
			try
			{
				// Status of timer can be changed. So it must be checked
				// just before execution. If timer is waiting for
				// deregistration it must not be executed.
				if( timer_status::wait_for_execution == head->m_status )
					head->m_action.exec();
			}
			catch( const std::exception & x )
			{
				this->m_exception_handler( x );
			}
			catch( ... )
			{
				std::ostringstream ss;
				ss << __FILE__ << "(" << __LINE__ 
					<< "): an unknown exception from timer action";
				this->m_error_logger( ss.str() );
				std::abort();
			}

			head = head->m_next;
		}

		lock.lock();
	}

	/*!
	 * \brief Process list of elapsed timers after execution of
	 * its actions.
	 *
	 * Active periodic timers will be rescheduled. All other timers
	 * will be deactivated and removed.
	 */
	void
	utilize_exec_list(
		//! Head of execution list.
		//! Cannot be null.
		timer_type * head )
	{
		while( head )
		{
			auto t = head;
			head = head->m_next;

			// Actual periodic timer must be rescheduled.
			if( timer_status::wait_for_execution == t->m_status &&
					monotonic_clock::duration::zero() != t->m_period )
			{
				t->m_when += t->m_period;
				t->m_status = timer_status::active;

				insert_timer_to_list( t );
			}
			else
			{
				// Timer must be utilized.
				this->dec_timer_count( t->kind() );
				t->m_status = timer_status::deactivated;
				timer_object< Thread_Safety >::decrement_references( t );
			}
		}
	}
};

//
// timer_heap_engine_defaults
//
/*!
 * \brief Container for static method with default values for
 * timer_heap engine.
 *
 * \since
 * v.1.1.0
 */
struct timer_heap_engine_defaults
{
	//! Default initial capacity of heap-array.
	inline static std::size_t
	default_initial_heap_capacity() { return 64; }
};

//
// timer_heap_engine
//

/*!
 * \brief An engine for timer heap mechanism.
 *
 * This timer engine uses timer mechanism based on
 * <a href="http://en.wikipedia.org/wiki/Heap_%28data_structure%29">heap data structure</a>.
 * The timer with the earlier time point is on the top of
 * the heap. When this timer elapsed and removed next timer with the
 * eralier time point is going to the top of the heap.
 *
 * This implementation uses array-based <a
 * href="http://en.wikipedia.org/wiki/Binary_heap">binary heap</a>. The array
 * is growing as necessary to hold all the timers. The initial size of that
 * array can be specified in the constructor.
 *
 * \note Unlike timer_wheel and timer_list threads this thread unlock and
 * lock its mutex for processing every timers. It means that processing
 * speed of this thread will be slower then for timer_wheel or
 * timer_list threads. But this type of thread doesn't consume resources
 * when there is no timers (unlike timer_wheel thread). And has very
 * efficient activation and deactivation procedures (unlike timer_list
 * thread).
 *
 * \tparam Thread_Safety Thread-safety indicator.
 * Must be timertt::thread_safety::unsafe or timertt::thread_safety::safe.
 *
 * \tparam Timer_Action type of functor to perform an user-defined
 * action when timer expires. This must be Moveable and MoveConstructible
 * type.
 *
 * \tparam Error_Logger type of logger for errors detected during
 * timer thread execution. Interface for error logger is defined
 * by default_error_logger class.
 *
 * \tparam Actor_Exception_Handler type of handler for dealing with
 * exceptions thrown from timer actors. Interface for exception handler
 * is defined by default_actor_exception_handler.
 */
template<
	typename Thread_Safety,
	typename Timer_Action,
	typename Error_Logger,
	typename Actor_Exception_Handler >
class timer_heap_engine
	:	public engine_common<
			Thread_Safety, Timer_Action, Error_Logger, Actor_Exception_Handler >
{
	//! An alias for base class.
	using base_type = engine_common<
			Thread_Safety, Timer_Action, Error_Logger, Actor_Exception_Handler >;

	// Forward declaration.
	struct timer_type;

public :
	//! Type with default parameters for this engine.
	typedef timer_heap_engine_defaults defaults_type;

	//! Alias for timer_action type.
	using timer_action = typename base_type::timer_action;	

	//! Alias for scoped timer object.
	using scoped_timer_object =
			scoped_timer_object_holder< timer_type >;

	//! Constructor with all parameters.
	timer_heap_engine(
		//! An initial size for heap array.
		std::size_t initial_heap_capacity,
		//! An error logger for timer thread.
		Error_Logger error_logger,
		//! An actor exception handler for timer thread.
		Actor_Exception_Handler exception_handler )
		:	base_type( error_logger, exception_handler )
	{
		m_heap.reserve( initial_heap_capacity );
	}

	~timer_heap_engine()
	{
		clear_all();
	}

	//! Create timer to be activated later.
	timer_object_holder< Thread_Safety >
	allocate()
	{
		return timer_object_holder< Thread_Safety >( new timer_type() );
	}

	//! Activate timer and schedule it for execution.
	/*!
	 * \return true is new timer is a timer on the top of the heap
	 * (has earlier expiration time).
	 *
	 * \throw std::exception If timer thread is not started.
	 * \throw std::exception If \a timer is already activated.
	 *
	 * \tparam Duration_1 actual type which represents time duration.
	 * \tparam Duration_2 actual type which represents time duration.
	 */
	template< class Duration_1, class Duration_2 >
	bool
	activate(
		//! Timer to be activated.
		timer_object_holder< Thread_Safety > timer,
		//! Pause for timer execution.
		Duration_1 pause,
		//! Repetition period.
		//! If <tt>Duration_2::zero() == period</tt> then timer will be
		//! single-shot.
		Duration_2 period,
		//! Action for the timer.
		timer_action action )
	{
		auto heap_timer = timer.template cast_to< timer_type >();
		ensure_timer_deactivated( heap_timer );

		// Timer object must be correctly (re)initialized.
		heap_timer->m_action.assign( std::move( action ) );
		heap_timer->m_when = monotonic_clock::now() + pause;
		heap_timer->m_period = std::chrono::duration_cast<
				monotonic_clock::duration >( period );

		// Timer must be taken under control.
		timer_object< Thread_Safety >::increment_references( heap_timer );

		// Timer will be marked as active during insertion into
		// heap structure.
		heap_add( heap_timer );

		// Count of timers must be incremented.
		this->inc_timer_count( heap_timer->kind() );

		return heap_timer == heap_head();
	}

	/*!
	 * \brief Perform an attempt to reschedule a timer.
	 *
	 * Before v.1.2.1 there was just one way to reschedule a timer:
	 * method deactivate() must be called first and then method
	 * activate() must be called for the same timer. This approach is
	 * not fast because in the case of thread-safe engines it requires
	 * two operations on a mutex.
	 *
	 * Since v.1.2.1 there is a reschedule() method which does deactivation
	 * of a timer (if it is active) and then new activation of this timer.
	 * All actions are performed by using just one operation on a mutex.
	 *
	 * \note
	 * This operation can fail if the timer to be rescheduled is in processing.
	 * Because of that it is recommended to use such operation for
	 * timer_managers only. But even with timer_managers this operation
	 * should be used with care.
	 *
	 * \attention
	 * It move operator for a timer_action throws then timer will be
	 * deactivated. The state for a timer_action itself will be unknown.
	 * 
	 * \throw std::exception If timer thread is not started.
	 * \throw std::exception If \a timer is in processing right now.
	 *
	 * \tparam Duration_1 actual type which represents time duration.
	 * \tparam Duration_2 actual type which represents time duration.
	 *
	 * \since
	 * v.1.2.1
	 */
	template< class Duration_1, class Duration_2 >
	bool
	reschedule(
		//! Timer to be rescheduled.
		timer_object_holder< Thread_Safety > timer,
		//! Pause for timer execution.
		Duration_1 pause,
		//! Repetition period.
		//! If <tt>Duration_2::zero() == period</tt> then timer will be
		//! single-shot.
		Duration_2 period,
		//! Action for the timer.
		timer_action action )
	{
		auto heap_timer = timer.template cast_to< timer_type >();
		// If timer is deactivated the usual activation logic can be used.
		if( heap_timer->deactivated() )
			return this->activate(
					std::move(timer), pause, period, std::move(action) );
		else if( heap_timer == m_timer_in_processing )
		{
			// Timer which is in processing now can't be reactivated.
			throw std::runtime_error( "timer is in processing now, "
					"it can't be rescheduled" );
		}

		// Timer must be removed from heap first.
		heap_remove( heap_timer );
		// Count of timers changed.
		this->dec_timer_count( heap_timer->kind() );

		// Timer object must be correctly (re)initialized.
		// If this assigment throws then we must deactivate the timer.
		try
		{
			heap_timer->m_action.assign( std::move(action) );
		}
		catch(...)
		{
			heap_timer->deactivate();
			timer_object< Thread_Safety >::decrement_references( heap_timer );
			// Exception must be rethrown;
			throw;
		}

		heap_timer->m_when = monotonic_clock::now() + pause;
		heap_timer->m_period = std::chrono::duration_cast<
				monotonic_clock::duration >( period );

		// Timer will be marked as active during insertion into
		// heap structure.
		heap_add( heap_timer );

		// Count of timers must be incremented.
		this->inc_timer_count( heap_timer->kind() );

		return heap_timer == heap_head();
	}

	//! Deactivate timer and remove it from the list.
	void
	deactivate(
		//! Timer to be deactivated.
		timer_object_holder< Thread_Safety > timer )
	{
		auto heap_timer = timer.template cast_to< timer_type >();
		if( !heap_timer->deactivated() )
		{
			// If this timer is not in processing now it can
			// be safely destroyed.
			if( heap_timer != m_timer_in_processing )
			{
				heap_remove( heap_timer );

				// Count of timers changed.
				this->dec_timer_count( heap_timer->kind() );

				// We can deactivate timer only after removing.
				// Because deactivation drops actual timer position.
				heap_timer->deactivate();

				// Release timer object.
				timer_object< Thread_Safety >::decrement_references( heap_timer );
			}
			else
			{
				// Otherwise m_timer_in_processing will be destroyed
				// after end of timer action processing.
				// But it must be deactivated right now.
				heap_timer->deactivate();
			}
		}
	}

	/*!
	 * \brief Process all expired timers from the heap.
	 *
	 * \note time_point for timers expiration detection is got
	 * only once at the begining on the method.
	 *
	 * \note \a lock unlocked and then locked back for every
	 * timer action execution.
	 */
	template< typename Unique_Lock >
	void
	process_expired_timers(
		//! Object's lock.
		Unique_Lock & lock )
	{
		// Process timers in loop until there are elapsed timers.
		const auto now = monotonic_clock::now();
		while( !heap_empty() && now > heap_head()->m_when )
		{
			m_timer_in_processing = heap_head();
			heap_remove( m_timer_in_processing );

			execute_timer_in_processing( lock );

			// If timer has become deactive it must be removed even
			// it is periodic timer.
			if( m_timer_in_processing->deactivated() ||
					m_timer_in_processing->single_shot() )
			{
				// Count of timers changed.
				this->dec_timer_count( m_timer_in_processing->kind() );

				m_timer_in_processing->deactivate();
				timer_object< Thread_Safety >::decrement_references(
						m_timer_in_processing );
			}
			else
			{
				// This is active periodic timer and it must be resheduled.
				m_timer_in_processing->m_when +=
						m_timer_in_processing->m_period;
				heap_add( m_timer_in_processing );
			}

			m_timer_in_processing = nullptr;
		}
	}

	/*!
	 * \brief Is empty timer list?
	 *
	 * \return true if there is no more timers.
	 */
	bool
	empty() const
	{
		return heap_empty();
	}

	/*!
	 * \brief Get time point of the next timer.
	 *
	 * \attention Must be called only when \a !empty().
	 */
	monotonic_clock::time_point
	nearest_time_point() const
	{
		return heap_head()->m_when;
	}

	//! Clear all timer demands.
	void
	clear_all()
	{
		for( auto t : m_heap )
		{
			t->deactivate();
			timer_object< Thread_Safety >::decrement_references( t );
		}

		this->reset_timer_count();
		m_heap.clear();
	}

private :
	//! Type of heap timer.
	struct timer_type : public timer_object< Thread_Safety >
	{
		//! A special value which means that timer is deactivated.
		/*!
		 * This value is illegal index in heap-array because
		 * position numbers in heap-array are started from 1, not from 0.
		 */
		static const std::size_t deactivation_indicator = 0;

		//! Time of execution for this timer.
		monotonic_clock::time_point m_when;

		//! Period in ticks.
		/*!
		 * Zero means that demand is single shot.
		 */
		monotonic_clock::duration m_period;

		//! Timer action.
		timer_action_holder< timer_action > m_action;

		//! Position in the heap-array.
		std::size_t m_position = deactivation_indicator;

		//! Is timer deactivated.
		bool
		deactivated() const 
		{
			return deactivation_indicator == m_position;
		}

		//! Set deactivation indicator on.
		void
		deactivate()
		{
			m_position = deactivation_indicator;
		}

		//! Is this is single shot timer?
		bool
		single_shot() const
		{
			return monotonic_clock::duration::zero() == m_period;
		}

		/*!
		 * \brief Detect type of timer (single-shot or periodic).
		 *
		 * \since
		 * v.1.1.1
		 */
		timer_kind
		kind() const
		{
			return single_shot() ?
					timer_kind::single_shot : timer_kind::periodic;
		}
	};

	/*!
	 * \name Object's attributes.
	 * \{
	 */
	//! Array for holding heap data structure.
	std::vector< timer_type * > m_heap;

	//! Timer which is currently in processing.
	timer_type * m_timer_in_processing = nullptr;
	/*!
	 * \}
	 */

	/*!
	 * \brief Hard check for deactivation state of the timer.
	 *
	 * \throw std::runtimer_error if timer is not deactivated.
	 */
	static void
	ensure_timer_deactivated( const timer_type * timer )
	{
		if( !timer->deactivated() )
			throw std::runtime_error( "timer is not in 'deactivated' state" );
	}

	//! Execute the current timer.
	template< class Unique_Lock >
	void
	execute_timer_in_processing(
		//! Object lock.
		//! This lock will be unlocked before execution of actions
		//! and locked back after.
		Unique_Lock & lock )
	{
		lock.unlock();

		try
		{
			m_timer_in_processing->m_action.exec();
		}
		catch( const std::exception & x )
		{
			this->m_exception_handler( x );
		}
		catch( ... )
		{
			std::ostringstream ss;
			ss << __FILE__ << "(" << __LINE__ 
				<< "): an unknown exception from timer action";
			this->m_error_logger( ss.str() );
			std::abort();
		}

		lock.lock();
	}

	/*!
	 * \name Methods for work with heap data structure.
	 * \{
	 */
	//! Is heap data structure empty?
	bool
	heap_empty() const
	{
		return m_heap.empty();
	}

	//! Get the minimal timer.
	/*!
	 * \attention This method must be called only on non-empty heap.
	 */
	timer_type *
	heap_head() const
	{
		return m_heap.front();
	}

	//! Add new timer to the heap data structure.
	void
	heap_add( timer_type * timer )
	{
		timer->m_position = m_heap.size() + 1;
		m_heap.push_back( timer );

		while( 1 != timer->m_position )
		{
			auto parent = heap_item( timer->m_position / 2 );
			if( parent->m_when > timer->m_when )
			{
				// timer must be heap-up on the place of the parent node.
				heap_swap( timer, parent );
			}
			else
				// There is no need to modify heap structure anymore.
				break;
		}
	}

	//! Remove timer from the heap data structure.
	void
	heap_remove( timer_type * timer )
	{
		if( timer->m_position == m_heap.size() )
			// A special case: timer to remove is a last added item
			// in the heap. It could be simply removed from heap
			// without any other actions.
			m_heap.pop_back();
		else
		{
			auto last_item = m_heap.back();
			heap_swap( timer, last_item );
			m_heap.pop_back();

			// last_item must be heap-down to the appropriate place.
			while( true )
			{
				auto left_index = last_item->m_position * 2;
				auto right_index = left_index + 1;
				auto min_index = last_item->m_position;

				if( left_index <= m_heap.size() &&
						heap_item( left_index )->m_when <=
								heap_item( min_index )->m_when )
					min_index = left_index;

				if( right_index <= m_heap.size() &&
						heap_item( right_index )->m_when <=
								heap_item( min_index )->m_when )
					min_index = right_index;

				if( min_index != last_item->m_position )
					heap_swap( last_item, heap_item( min_index ) );
				else
					// Heap structure is correct.
					break;
			}
		}
	}

	//! Swap two heap nodes.
	void
	heap_swap( timer_type * a, timer_type * b )
	{
		m_heap[ a->m_position - 1 ] = b;
		m_heap[ b->m_position - 1 ] = a;

		std::swap( a->m_position, b->m_position );
	}

	//! Get timer by it index.
	/*!
	 * This accessor work with respect that positions are started from 1.
	 */
	timer_type *
	heap_item( std::size_t position ) const
	{
		return m_heap[ position - 1 ];
	}
	/*!
	 * \}
	 */
};

//
// thread_unsafe_manager_mixin
//

/*!
 * \brief A mixin which must be used as base class for not-thread-safe
 * timer managers.
 *
 * \since
 * v.1.1.0
 */
struct thread_unsafe_manager_mixin
{
	//! A empty class for an object's lock emulation.
	/*!
	 * Instance of that class will be used in not-thread-safe
	 * code in places where object's lock is necessary.
	 *
	 * Because this class is empty its usage will be removed by
	 * optimized compiler.
	 */
	class lock_guard
	{
	public :
		lock_guard( thread_unsafe_manager_mixin & ) {}

		void lock() {}
		void unlock() {}
	};

	void
	ensure_started() {}

	void
	notify() {}
};

//
// thread_safe_manager_mixin
//
/*!
 * \brief A mixin which must be used as base class for thread-safe
 * timer managers.
 *
 * \since
 * v.1.1.0
 */
struct thread_safe_manager_mixin
{
	//! Timer manager's lock.
	std::mutex m_lock;

	//! A special wrapper around actual std::unique_lock.
	class lock_guard
	{
		std::unique_lock< std::mutex > m_lock;

	public :
		lock_guard( thread_safe_manager_mixin & self )
			: m_lock( self.m_lock )
		{}

		void lock() { m_lock.lock(); }
		void unlock() { m_lock.unlock(); }
	};

	void
	ensure_started() {}

	void
	notify() {}
};

//
// thread_mixin
//

/*!
 * \brief A mixin which must be used as base class for timer threads.
 *
 * \since
 * v.1.1.0
 */
struct thread_mixin
{
	//! Timer thread's lock.
	std::mutex m_lock;

	//! Condition variable for waiting for next event.
	std::condition_variable m_condition;

	//! Underlying thread object.
	/*!
	 * \note Will be created during timer thread start and
	 * destroyed after timer thread shutdown.
	 */
	std::shared_ptr< std::thread > m_thread;

	//! A special wrapper around actual std::unique_lock.
	class lock_guard
	{
		std::unique_lock< std::mutex > m_lock;

	public :
		lock_guard( thread_mixin & self )
			: m_lock( self.m_lock )
		{}

		void lock() { m_lock.lock(); }
		void unlock() { m_lock.unlock(); }

		//! Accessor for actual std::unique_lock object.
		std::unique_lock< std::mutex > &
		actual_lock() { return m_lock; }
	};

	//! Checks that timer thread is really started.
	/*!
	 * \throw std::exception if thread is not started.
	 */
	void
	ensure_started()
	{
		if( !m_thread )
			throw std::runtime_error( "timer thread is not started" );
	}

	//! Sends notification to timer thread.
	void
	notify()
	{
		m_condition.notify_one();
	}
};

/*!
 * \brief A type-container for types of engine-consumers.
 *
 * \since
 * v.1.1.0
 */
struct consumer_type
{
	/*!
	 * \brief Indicator that an engine will be owned by timer manager.
	 *
	 * \since
	 * v.1.1.0
	 */
	struct manager {};
	/*!
	 * \brief Indicator that an engine will be owned by timer thread.
	 *
	 * \since
	 * v.1.1.0
	 */
	struct thread {};
};

/*!
 * \brief A selector of actual mixin type for timer manager or timer thread.
 *
 * \attention This type is empty because all actual content will be
 * defined in specializations.
 *
 * \since
 * v.1.1.0
 */
template< typename Thread_Safety, typename Consumer >
struct mixin_selector
{
};

/*!
 * \brief A selector of actual mixin type for not-thread-safe timer manager.
 *
 * \since
 * v.1.1.0
 */
template<>
struct mixin_selector< thread_safety::unsafe, consumer_type::manager >
{
	//! Actual type of the mixin.
	using type = thread_unsafe_manager_mixin;
};

/*!
 * \brief A selector of actual mixin type for thread-safe timer manager.
 *
 * \since
 * v.1.1.0
 */
template<>
struct mixin_selector< thread_safety::safe, consumer_type::manager >
{
	//! Actual type of the mixin.
	using type = thread_safe_manager_mixin;
};

/*!
 * \brief A selector of actual mixin type for timer thread.
 *
 * \since
 * v.1.1.0
 */
template<>
struct mixin_selector< thread_safety::safe, consumer_type::thread >
{
	//! Actual type of the mixin.
	using type = thread_mixin;
};

//
// basic_methods_impl_mixin
//

/*!
 * \brief A implementation of basic methods for timer managers and
 * timer threads.
 *
 * \tparam Engine actual type of engine to be used.
 * \tparam Consumer type of engine consumer (e.g. consumer_type::manager or
 * consumer_type::thread).
 *
 * \since
 * v.1.1.0
 */
template<
	typename Engine,
	typename Consumer >
class basic_methods_impl_mixin
	:	protected mixin_selector< typename Engine::thread_safety, Consumer >::type
	,	public Engine::defaults_type
{
	//! Shorthand for actual mixin type.
	using mixin_type = typename mixin_selector<
			typename Engine::thread_safety, Consumer >::type;

public :
	/*!
	 * \brief A typedef for thread safety type from Engine.
	 *
	 * \since
	 * v.1.1.2
	 */
	using thread_safety = typename Engine::thread_safety;

	//! An alias for timer_action type.
	using timer_action = typename Engine::timer_action;

	//! An alias for scoped timer objects.
	using scoped_timer_object = typename Engine::scoped_timer_object;

	//! Shorthand for timer objects' smart pointer.
	/*!
	 * \note
	 * Since v.1.2.1 it is a public type name.
	 */
	using timer_holder = timer_object_holder< typename Engine::thread_safety >;

	//! Constructor with all parameters.
	template< typename... Args >
	basic_methods_impl_mixin(
		Args && ... args )
		:	m_engine( std::forward< Args >(args)... )
	{
	}

	/*!
	 * \brief Allocate of new timer object.
	 *
	 * \note This object must be activated by activate() methods.
	 */
	timer_holder
	allocate()
	{
		return m_engine.allocate();
	}

	//! Activate timer and schedule it for execution.
	/*!
	 *
	 * \throw std::exception If timer thread is not started.
	 * \throw std::exception If \a timer is already activated.
	 *
	 * \tparam Duration_1 actual type which represents time duration.
	 */
	template< class Duration_1 >
	void
	activate(
		//! Timer to be activated.
		timer_holder timer,
		//! Pause for timer execution.
		Duration_1 pause,
		//! Action for the timer.
		timer_action action )
	{
		activate(
				std::move( timer ),
				pause,
				monotonic_clock::duration::zero(),
				std::move( action ) );
	}

	/*!
	 * \brief Perform an attempt to reschedule a timer.
	 *
	 * Before v.1.2.1 there was just one way to reschedule a timer:
	 * method deactivate() must be called first and then method
	 * activate() must be called for the same timer. This approach is
	 * not fast because in the case of thread-safe engines it requires
	 * two operations on a mutex.
	 *
	 * Since v.1.2.1 there is a reschedule() method which does deactivation
	 * of a timer (if it is active) and then new activation of this timer.
	 * All actions are performed by using just one operation on a mutex.
	 *
	 * \note
	 * This operation can fail if the timer to be rescheduled is in processing.
	 * Because of that it is recommended to use such operation for
	 * timer_managers only. But even with timer_managers this operation
	 * should be used with care.
	 *
	 * \attention
	 * It move operator for a timer_action throws then timer will be
	 * deactivated. The state for a timer_action itself will be unknown.
	 * 
	 * \throw std::exception If timer thread is not started.
	 * \throw std::exception If \a timer is in processing right now.
	 *
	 * \tparam Duration_1 actual type which represents time duration.
	 *
	 * \since
	 * v.1.2.1
	 */
	template< class Duration_1 >
	void
	reschedule(
		//! Timer to be rescheduled.
		timer_holder timer,
		//! Pause for timer execution.
		Duration_1 pause,
		//! Action for the timer.
		timer_action action )
	{
		reschedule(
				std::move( timer ),
				pause,
				monotonic_clock::duration::zero(),
				std::move( action ) );
	}

	//! Activate a scoped timer and schedule it for execution.
	/*!
	 *
	 * \note
	 * A proper lifetime of this timer must be controlled by user.
	 *
	 * \throw std::exception If timer thread is not started.
	 * \throw std::exception If \a timer is already activated.
	 *
	 * \tparam Duration_1 actual type which represents time duration.
	 *
	 * \since
	 * v.1.2.0
	 */
	template< class Duration_1 >
	void
	activate(
		//! Timer to be activated.
		scoped_timer_object & timer,
		//! Pause for timer execution.
		Duration_1 pause,
		//! Action for the timer.
		timer_action action )
	{
		this->activate( timer_holder{timer}, pause, action );
	}

	//! Activate timer and schedule it for execution.
	/*!
	 * There is no need to preallocate timer object. It will
	 * be allocated automatically, but not be shown to user.
	 *
	 * \throw std::exception If timer thread is not started.
	 *
	 * \tparam Duration_1 actual type which represents time duration.
	 */
	template< class Duration_1 >
	void
	activate(
		//! Pause for timer execution.
		Duration_1 pause,
		//! Action for the timer.
		timer_action action )
	{
		activate(
				allocate(),
				pause,
				monotonic_clock::duration::zero(),
				std::move( action ) );
	}

	//! Activate timer and schedule it for execution.
	/*!
	 *
	 * \throw std::exception If timer thread is not started.
	 * \throw std::exception If \a timer is already activated.
	 *
	 * \tparam Duration_1 actual type which represents time duration.
	 * \tparam Duration_2 actual type which represents time duration.
	 */
	template< class Duration_1, class Duration_2 >
	void
	activate(
		//! Timer to be activated.
		timer_holder timer,
		//! Pause for timer execution.
		Duration_1 pause,
		//! Repetition period.
		//! If <tt>Duration_2::zero() == period</tt> then timer will be
		//! single-shot.
		Duration_2 period,
		//! Action for the timer.
		timer_action action )
	{
		typename mixin_type::lock_guard locker{ *this };

		this->ensure_started();

		if( m_engine.activate(
				std::move( timer ), pause, period, std::move( action ) ) )
			this->notify();
	}

	/*!
	 * \brief Perform an attempt to reschedule a timer.
	 *
	 * Before v.1.2.1 there was just one way to reschedule a timer:
	 * method deactivate() must be called first and then method
	 * activate() must be called for the same timer. This approach is
	 * not fast because in the case of thread-safe engines it requires
	 * two operations on a mutex.
	 *
	 * Since v.1.2.1 there is a reschedule() method which does deactivation
	 * of a timer (if it is active) and then new activation of this timer.
	 * All actions are performed by using just one operation on a mutex.
	 *
	 * \note
	 * This operation can fail if the timer to be rescheduled is in processing.
	 * Because of that it is recommended to use such operation for
	 * timer_managers only. But even with timer_managers this operation
	 * should be used with care.
	 *
	 * \attention
	 * It move operator for a timer_action throws then timer will be
	 * deactivated. The state for a timer_action itself will be unknown.
	 * 
	 * \throw std::exception If timer thread is not started.
	 * \throw std::exception If \a timer is in processing right now.
	 *
	 * \tparam Duration_1 actual type which represents time duration.
	 * \tparam Duration_2 actual type which represents time duration.
	 *
	 * \since
	 * v.1.2.1
	 */
	template< class Duration_1, class Duration_2 >
	void
	reschedule(
		//! Timer to be activated.
		timer_holder timer,
		//! Pause for timer execution.
		Duration_1 pause,
		//! Repetition period.
		//! If <tt>Duration_2::zero() == period</tt> then timer will be
		//! single-shot.
		Duration_2 period,
		//! Action for the timer.
		timer_action action )
	{
		typename mixin_type::lock_guard locker{ *this };

		this->ensure_started();

		if( m_engine.reschedule(
				std::move( timer ), pause, period, std::move( action ) ) )
			this->notify();
	}

	//! Activate a scoped timer and schedule it for execution.
	/*!
	 *
	 * \note
	 * A proper lifetime of this timer must be controlled by user.
	 *
	 * \throw std::exception If timer thread is not started.
	 * \throw std::exception If \a timer is already activated.
	 *
	 * \tparam Duration_1 actual type which represents time duration.
	 * \tparam Duration_2 actual type which represents time duration.
	 *
	 * \since
	 * v.1.2.0
	 */
	template< class Duration_1, class Duration_2 >
	void
	activate(
		//! Timer to be activated.
		scoped_timer_object & timer,
		//! Pause for timer execution.
		Duration_1 pause,
		//! Repetition period.
		//! If <tt>Duration_2::zero() == period</tt> then timer will be
		//! single-shot.
		Duration_2 period,
		//! Action for the timer.
		timer_action action )
	{
		this->activate( timer_holder{timer}, pause, period, std::move(action) );
	}

	//! Activate timer and schedule it for execution.
	/*!
	 * There is no need to preallocate timer object. It will
	 * be allocated automatically, but not be shown to user.
	 *
	 * \throw std::exception If timer thread is not started.
	 *
	 * \tparam Duration_1 actual type which represents time duration.
	 * \tparam Duration_2 actual type which represents time duration.
	 */
	template< class Duration_1, class Duration_2 >
	void
	activate(
		//! Pause for timer execution.
		Duration_1 pause,
		//! Repetition period.
		//! If <tt>Duration_2::zero() == period</tt> then timer will be
		//! single-shot.
		Duration_2 period,
		//! Action for the timer.
		timer_action action )
	{
		activate( allocate(), pause, period, std::move( action ) );
	}

	//! Deactivate timer and remove it from the list.
	void
	deactivate(
		//! Timer to be deactivated.
		timer_holder timer )
	{
		typename mixin_type::lock_guard locker{ *this };

		m_engine.deactivate( timer );
	}

	//! Deactivate timer and remove it from the list.
	/*!
	 * \since
	 * v.1.2.0
	 */
	void
	deactivate(
		//! Timer to be deactivated.
		scoped_timer_object & timer )
	{
		this->deactivate( timer_holder{timer} );
	}

	/*!
	 * \brief Count of timers of various types.
	 *
	 * \note This are quantities of all timers known to manager/thread.
	 * Some of them can be already deactivated but not removed yet.
	 *
	 * \since
	 * v.1.1.1
	 */
	timer_quantities
	get_timer_quantities()
	{
		typename mixin_type::lock_guard locker{ *this };

		return m_engine.get_timer_quantities();
	}

	/*!
	 * \brief Check for emptiness.
	 *
	 * \since
	 * v.1.1.3
	 */
	bool
	empty()
	{
		typename mixin_type::lock_guard locker{ *this };

		return m_engine.empty();
	}

protected :
	//! Actual timer engine instance.
	Engine m_engine;
};

//
// manager_impl_template
//

/*!
 * \brief Template-based implementation of timer manager.
 *
 * \tparam Engine actual type of engine to be used.
 *
 * \since
 * v.1.1.0
 */
template< typename Engine >
class manager_impl_template
	:	public basic_methods_impl_mixin< Engine, consumer_type::manager > 
{
	//! Shorthand for base type.
	using base_type = basic_methods_impl_mixin<
			Engine,
			consumer_type::manager >;

public :
	//! Constructor with all parameters.
	template< typename... Args >
	manager_impl_template(
		Args && ... args )
		:	base_type( std::forward< Args >(args)... )
	{
	}

	//! Reset all timers and return manager to the initial state.
	void
	reset()
	{
		typename manager_impl_template::lock_guard locker{ *this };
		this->m_engine.clear_all();
	}

	//! Perform processing of expired timers.
	void
	process_expired_timers()
	{
		typename manager_impl_template::lock_guard locker{ *this };

		this->m_engine.process_expired_timers( locker );
	}

	//! Get the time for next process_expired_timers invocation.
	/*!
	 * \return tuple<true,timepoint> if there is a timer to process. Or
	 * tuple<false,undefined> if there is no timers to be processed.
	 */
	std::tuple< bool, monotonic_clock::time_point >
	nearest_time_point()
	{
		typename manager_impl_template::lock_guard locker{ *this };
		auto e = this->m_engine.empty();
		if( !e )
			return std::make_tuple( true, this->m_engine.nearest_time_point() );
		else
			return std::make_tuple( false, monotonic_clock::time_point() );
	}

	//! Get the sleeping time before the earlist timer expiration.
	/*!
	 * \return actual sleeping time if there is at least one timer.
	 * Or \a default_timeout if there is no any timers.
	 *
	 * \tparam Duration type for \a default_timeout
	 */
	template< typename Duration >
	monotonic_clock::duration
	timeout_before_nearest_timer(
		//! Default timeout value which will be used if there is no any timers.
		Duration default_timeout )
	{
		auto r = this->nearest_time_point();
		if( std::get<0>( r ) )
		{
			auto now = monotonic_clock::now();
			const auto & f = std::get<1>( r );
			if( now > f )
				return monotonic_clock::duration( 0 );
			else
				return (f - now);
		}
		else
			return default_timeout;
	}
};

//
// thread_impl_template
//

/*!
 * \brief Template-based implementation of timer thread.
 *
 * \tparam Engine actual type of engine to be used.
 *
 * \since
 * v.1.1.0
 */
template< typename Engine >
class thread_impl_template
	:	public basic_methods_impl_mixin< Engine, consumer_type::thread > 
{
	//! Shorthand for base type.
	using base_type = basic_methods_impl_mixin<
			Engine,
			consumer_type::thread >;

public :
	//! Constructor with all parameters.
	template< typename... Args >
	thread_impl_template(
		Args && ... args )
		:	base_type( std::forward< Args >(args)... )
	{
	}

	//! Destructor.
	/*!
	 * Calls shutdown_and_join()
	 */
	~thread_impl_template()
	{
		shutdown_and_join();
	}

	//! Start timer thread.
	/*!
	 * \throw std::runtime_error if thread is already started.
	 */
	void
	start()
	{
		typename base_type::lock_guard locker{ *this };

		if( this->m_thread )
			throw std::runtime_error( "timer thread is already started" );
		else
			this->m_shutdown = false;

		this->m_thread = std::make_shared< std::thread >(
				std::bind( &thread_impl_template::body, this ) );
	}

	//! Initiate shutdown for the timer thread without waiting for completion.
	void
	shutdown()
	{
		typename base_type::lock_guard locker{ *this };

		if( this->m_thread && !this->m_shutdown )
		{
			this->m_shutdown = true;
			this->notify();
		}
	}

	//! Wait for completion of timer thread.
	/*!
	 * Method shutdown() must be called somewhere else.
	 */
	void
	join()
	{
		std::shared_ptr< std::thread > t;
		{
			typename base_type::lock_guard locker{ *this };
			t = this->m_thread;
		}
		if( t )
		{
			t->join();

			typename base_type::lock_guard locker{ *this };
			this->m_thread.reset();
		}
	}

	//! Initiate shutdown and wait for completion.
	void
	shutdown_and_join()
	{
		shutdown();
		join();
	}

protected :
	/*!
	 * \name Object's attributes.
	 * \{
	 */
	//! Shutdown flag.
	bool m_shutdown = false;
	/*!
	 * \}
	 */

	//! Thread body.
	void
	body()
	{
		typename base_type::lock_guard locker{ *this };

		while( !this->m_shutdown )
		{
			this->m_engine.process_expired_timers( locker );

			sleep_for_next_event( locker );
		}

		this->m_engine.clear_all();
	}

	/*!
	 * \brief Waiting for next event to process.
	 *
	 * If the list is not emply the thread will sleep until
	 * time point of the first timer in the list.
	 */
	void
	sleep_for_next_event(
		//! Object's lock.
		//! The lock is necessary for waiting on condition variable.
		typename base_type::lock_guard & lock )
	{
		if( !this->m_shutdown )
		{
			if( !this->m_engine.empty() )
			{
				auto time_point = this->m_engine.nearest_time_point();
				this->m_condition.wait_until( lock.actual_lock(), time_point );
			}
			else
				this->m_condition.wait( lock.actual_lock() );
		}
	}
};

} /* namespace details */

//
// timer_wheel_thread_template
//

/*!
 * \brief A timer wheel thread template.
 *
 * Please see description of details::timer_wheel_engine for the details
 * of the timer wheel mechanism.
 *
 * \tparam Timer_Action type of functor to perform an user-defined
 * action when timer expires. This must be Moveable and MoveConstructible
 * type.
 *
 * \tparam Error_Logger type of logger for errors detected during
 * timer thread execution. Interface for error logger is defined
 * by default_error_logger class.
 *
 * \tparam Actor_Exception_Handler type of handler for dealing with
 * exceptions thrown from timer actors. Interface for exception handler
 * is defined by default_actor_exception_handler.
 */
template<
	typename Timer_Action,
	typename Error_Logger,
	typename Actor_Exception_Handler >
class timer_wheel_thread_template
	: public
		details::thread_impl_template<
				details::timer_wheel_engine<
						::timertt::thread_safety::safe,
						Timer_Action,
						Error_Logger,
						Actor_Exception_Handler > > 
{
	using base_type =
			details::thread_impl_template<
					details::timer_wheel_engine<
							::timertt::thread_safety::safe,
							Timer_Action,
							Error_Logger,
							Actor_Exception_Handler > >;

public :
	//! Default constructor.
	timer_wheel_thread_template()
		:	timer_wheel_thread_template(
				base_type::default_wheel_size(),
				base_type::default_granularity(),
				Error_Logger(),
				Actor_Exception_Handler() )
	{}

	//! Constructor with wheel size and granularity parameters.
	timer_wheel_thread_template(
		//! Size of the wheel.
		unsigned int wheel_size,
		//! Size of time step for the timer_wheel.
		monotonic_clock::duration granularity )
		:	timer_wheel_thread_template(
				wheel_size,
				granularity,
				Error_Logger(),
				Actor_Exception_Handler() )
	{}

	//! Constructor with all parameters.
	timer_wheel_thread_template(
		//! Size of the wheel.
		unsigned int wheel_size,
		//! Size of time step for the timer_wheel.
		monotonic_clock::duration granularity,
		//! An error logger for timer thread.
		Error_Logger error_logger,
		//! An actor exception handler for timer thread.
		Actor_Exception_Handler exception_handler )
		:	base_type(
				wheel_size,
				granularity,
				error_logger,
				exception_handler )
	{}
};

//
// timer_wheel_manager_template
//

/*!
 * \brief A timer wheel manager template.
 *
 * \note Please see description of details::timer_wheel_engine for the details
 * of the timer wheel mechanism.
 *
 * \tparam Thread_Safety Thread-safety indicator.
 * Must be timertt::thread_safety::unsafe or timertt::thread_safety::safe.
 *
 * \tparam Timer_Action type of functor to perform an user-defined
 * action when timer expires. This must be Moveable and MoveConstructible
 * type.
 *
 * \tparam Error_Logger type of logger for errors detected during
 * timer handling. Interface for error logger is defined
 * by default_error_logger class.
 *
 * \tparam Actor_Exception_Handler type of handler for dealing with
 * exceptions thrown from timer actors. Interface for exception handler
 * is defined by default_actor_exception_handler.
 *
 * \since
 * v.1.1.0
 */
template<
	typename Thread_Safety,
	typename Timer_Action = default_timer_action_type,
	typename Error_Logger = default_error_logger,
	typename Actor_Exception_Handler = default_actor_exception_handler >
class timer_wheel_manager_template
	: public
		details::manager_impl_template<
				details::timer_wheel_engine<
						Thread_Safety,
						Timer_Action,
						Error_Logger,
						Actor_Exception_Handler > > 
{
	//! Shorthand for base type.
	using base_type = 
			details::manager_impl_template<
					details::timer_wheel_engine<
							Thread_Safety,
							Timer_Action,
							Error_Logger,
							Actor_Exception_Handler > >;

public :
	//! Default constructor.
	timer_wheel_manager_template()
		:	timer_wheel_manager_template(
				base_type::default_wheel_size(),
				base_type::default_granularity(),
				Error_Logger(),
				Actor_Exception_Handler() )
	{}

	//! Constructor with wheel size and granularity parameters.
	timer_wheel_manager_template(
		//! Size of the wheel.
		unsigned int wheel_size,
		//! Size of time step for the timer_wheel.
		monotonic_clock::duration granularity )
		:	timer_wheel_manager_template(
				wheel_size,
				granularity,
				Error_Logger(),
				Actor_Exception_Handler() )
	{}

	//! Constructor with all parameters.
	timer_wheel_manager_template(
		//! Size of the wheel.
		unsigned int wheel_size,
		//! Size of time step for the timer_wheel.
		monotonic_clock::duration granularity,
		//! An error logger for timer thread.
		Error_Logger error_logger,
		//! An actor exception handler for timer thread.
		Actor_Exception_Handler exception_handler )
		:	base_type(
				wheel_size,
				granularity,
				error_logger,
				exception_handler )
	{}
};

//
// default_timer_wheel_thread
//
/*!
 * \brief Alias for timer_wheel_thread_template with the default
 * parameters.
 *
 * \since
 * v.1.2.0
 */
using default_timer_wheel_thread =
		timer_wheel_thread_template<
				default_timer_action_type,
				default_error_logger,
				default_actor_exception_handler >;

//
// timer_list_thread_template
//

/*!
 * \brief A timer list thread template.
 *
 * \note Please see description of details::timer_list_engine for the
 * details of this timer mechanism.
 *
 * \tparam Timer_Action type of functor to perform an user-defined
 * action when timer expires. This must be Moveable and MoveConstructible
 * type.
 *
 * \tparam Error_Logger type of logger for errors detected during
 * timer thread execution. Interface for error logger is defined
 * by default_error_logger class.
 *
 * \tparam Actor_Exception_Handler type of handler for dealing with
 * exceptions thrown from timer actors. Interface for exception handler
 * is defined by default_actor_exception_handler.
 */
template<
	typename Timer_Action,
	typename Error_Logger,
	typename Actor_Exception_Handler >
class timer_list_thread_template
	: public
		details::thread_impl_template<
				details::timer_list_engine<
						::timertt::thread_safety::safe,
						Timer_Action,
						Error_Logger,
						Actor_Exception_Handler > > 
{
	using base_type =
			details::thread_impl_template<
					details::timer_list_engine<
							::timertt::thread_safety::safe,
							Timer_Action,
							Error_Logger,
							Actor_Exception_Handler > >;

public :
	//! Default constructor.
	timer_list_thread_template()
	{}

	//! Constructor with all parameters.
	timer_list_thread_template(
		Error_Logger error_logger,
		Actor_Exception_Handler actor_exception_handler )
		:	base_type( error_logger, actor_exception_handler )
	{
	}
};

//
// default_timer_list_thread
//
/*!
 * \brief Alias for timer_list_thread_template with the default
 * parameters.
 *
 * \since
 * v.1.2.0
 */
using default_timer_list_thread =
		timer_list_thread_template<
				default_timer_action_type,
				default_error_logger,
				default_actor_exception_handler >;

//
// timer_list_manager_template
//

/*!
 * \brief A timer list thread template.
 *
 * \note Please see description of details::timer_list_engine for the
 * details of this timer mechanism.
 *
 * \tparam Thread_Safety Thread-safety indicator.
 * Must be timertt::thread_safety::unsafe or timertt::thread_safety::safe.
 *
 * \tparam Timer_Action type of functor to perform an user-defined
 * action when timer expires. This must be Moveable and MoveConstructible
 * type.
 *
 * \tparam Error_Logger type of logger for errors detected during
 * timer handling. Interface for error logger is defined
 * by default_error_logger class.
 *
 * \tparam Actor_Exception_Handler type of handler for dealing with
 * exceptions thrown from timer actors. Interface for exception handler
 * is defined by default_actor_exception_handler.
 *
 * \since
 * v.1.1.0
 */
template<
	typename Thread_Safety,
	typename Timer_Action = default_timer_action_type,
	typename Error_Logger = default_error_logger,
	typename Actor_Exception_Handler = default_actor_exception_handler >
class timer_list_manager_template
	: public
		details::manager_impl_template<
				details::timer_list_engine<
						Thread_Safety,
						Timer_Action,
						Error_Logger,
						Actor_Exception_Handler > > 
{
	using base_type =
			details::manager_impl_template<
					details::timer_list_engine<
							Thread_Safety,
							Timer_Action,
							Error_Logger,
							Actor_Exception_Handler > >;

public :
	//! Default constructor.
	timer_list_manager_template()
	{}

	//! Constructor with all parameters.
	timer_list_manager_template(
		Error_Logger error_logger,
		Actor_Exception_Handler actor_exception_handler )
		:	base_type( error_logger, actor_exception_handler )
	{
	}
};

//
// timer_heap_thread_template
//

/*!
 * \brief A timer heap thread template.
 *
 * \note Please see description of details::timer_heap_engine for the
 * details about this timer mechanism.
 *
 * \tparam Error_Logger type of logger for errors detected during
 * timer thread execution. Interface for error logger is defined
 * by default_error_logger class.
 *
 * \tparam Timer_Action type of functor to perform an user-defined
 * action when timer expires. This must be Moveable and MoveConstructible
 * type.
 *
 * \tparam Actor_Exception_Handler type of handler for dealing with
 * exceptions thrown from timer actors. Interface for exception handler
 * is defined by default_actor_exception_handler.
 */
template<
	typename Timer_Action,
	typename Error_Logger,
	typename Actor_Exception_Handler >
class timer_heap_thread_template
	: public
		details::thread_impl_template<
				details::timer_heap_engine<
						::timertt::thread_safety::safe,
						Timer_Action,
						Error_Logger,
						Actor_Exception_Handler > > 
{
	//! Shorthand for base type.
	using base_type =
			details::thread_impl_template<
					details::timer_heap_engine<
							::timertt::thread_safety::safe,
							Timer_Action,
							Error_Logger,
							Actor_Exception_Handler > >;

public :
	//! Default constructor.
	/*!
	 * Value default_initial_heap_capacity() is used as initial
	 * heap array size.
	 */
	timer_heap_thread_template()
		:	timer_heap_thread_template(
				base_type::default_initial_heap_capacity(),
				Error_Logger(),
				Actor_Exception_Handler() )
	{}

	//! Constructor to specify initial capacity of heap-array.
	timer_heap_thread_template(
		//! An initial size for heap array.
		std::size_t initial_heap_capacity )
		:	timer_heap_thread_template(
				initial_heap_capacity,
				Error_Logger(),
				Actor_Exception_Handler() )
	{}

	//! Constructor with all parameters.
	timer_heap_thread_template(
		//! An initial size for heap array.
		std::size_t initial_heap_capacity,
		//! An error logger for timer thread.
		Error_Logger error_logger,
		//! An actor exception handler for timer thread.
		Actor_Exception_Handler exception_handler )
		:	base_type(
				initial_heap_capacity,
				error_logger,
				exception_handler )
	{}
};

//
// default_timer_heap_thread
//
/*!
 * \brief Alias for timer_heap_thread_template with the default
 * parameters.
 *
 * \since
 * v.1.2.0
 */
using default_timer_heap_thread =
		timer_heap_thread_template<
				default_timer_action_type,
				default_error_logger,
				default_actor_exception_handler >;

//
// timer_heap_manager_template
//

/*!
 * \brief A timer heap manager template.
 *
 * \note Please see description of details::timer_heap_engine for the
 * details about this timer mechanism.
 *
 * \tparam Thread_Safety Thread-safety indicator.
 * Must be timertt::thread_safety::unsafe or timertt::thread_safety::safe.
 *
 * \tparam Timer_Action type of functor to perform an user-defined
 * action when timer expires. This must be Moveable and MoveConstructible
 * type.
 *
 * \tparam Error_Logger type of logger for errors detected during
 * timer handling. Interface for error logger is defined
 * by default_error_logger class.
 *
 * \tparam Actor_Exception_Handler type of handler for dealing with
 * exceptions thrown from timer actors. Interface for exception handler
 * is defined by default_actor_exception_handler.
 */
template<
	typename Thread_Safety,
	typename Timer_Action = default_timer_action_type,
	typename Error_Logger = default_error_logger,
	typename Actor_Exception_Handler = default_actor_exception_handler >
class timer_heap_manager_template
	: public
		details::manager_impl_template<
				details::timer_heap_engine<
						Thread_Safety,
						Timer_Action,
						Error_Logger,
						Actor_Exception_Handler > > 
{
	//! Shorthand for base type.
	using base_type =
		details::manager_impl_template<
				details::timer_heap_engine<
						Thread_Safety,
						Timer_Action,
						Error_Logger,
						Actor_Exception_Handler > >;

public :
	//! Default constructor.
	/*!
	 * Value default_initial_heap_capacity() is used as initial
	 * heap array size.
	 */
	timer_heap_manager_template()
		:	timer_heap_manager_template(
				base_type::default_initial_heap_capacity(),
				Error_Logger(),
				Actor_Exception_Handler() )
	{}

	//! Constructor to specify initial capacity of heap-array.
	timer_heap_manager_template(
		//! An initial size for heap array.
		std::size_t initial_heap_capacity )
		:	timer_heap_manager_template(
				initial_heap_capacity,
				Error_Logger(),
				Actor_Exception_Handler() )
	{}

	//! Constructor with all parameters.
	timer_heap_manager_template(
		//! An initial size for heap array.
		std::size_t initial_heap_capacity,
		//! An error logger for timer thread.
		Error_Logger error_logger,
		//! An actor exception handler for timer thread.
		Actor_Exception_Handler exception_handler )
		:	base_type(
				initial_heap_capacity,
				error_logger,
				exception_handler )
	{}
};

} /* namespace timertt */


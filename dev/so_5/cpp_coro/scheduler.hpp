/*
 * SObjectizer-5
 */

/// @file
/// @brief Declaration of scheduler interface for C++20 coroutines.
///
/// @since v.5.8.7

#if !defined(SO_5_CPP_CORO_SCHEDULER_HPP)
#define SO_5_CPP_CORO_SCHEDULER_HPP

#include <so_5/declspec.hpp>

#include <so_5/fwd.hpp>

#include <so_5/details/remaining_time_counter.hpp>

#include <atomic>
#include <chrono>
#include <coroutine>
#include <optional>
#include <utility>

namespace so_5::cpp_coro
{

//
// monotonic_clock_t
//
/// @brief An alias for steady_clock.
///
/// @since v.5.8.7
using monotonic_clock_t = std::chrono::steady_clock;

/// @brief Helper function to get a value for infinite sleep timeout.
///
/// This value will be used to detect the presence of the actual sleep timeout.
///
/// @since v.5.8.7
[[nodiscard]] inline constexpr
monotonic_clock_t::duration
infinite_speep_time() noexcept { return monotonic_clock_t::duration::max(); }

class scheduler_t;

/// @brief Enumeration of possible results of an attempt to suspend a
/// coroutine.
///
/// @since v.5.8.7
enum class try_suspend_result_t
	{
		/// The coroutine has to be suspended.
		should_be_suspended,
		/// The coroutine has to be resumed instead of the suspension.
		should_be_resumed
	};

//
// resumable_item_t
//
/*!
 * @brief Object to be used for communication between a scheduler and
 * the coroutine.
 *
 * It's a bridge between a coroutine and a scheduler to be used for
 * this coroutine. It holds references to the coroutine and the scheduler.
 *
 * The main goal is to avoid dynamic memory allocation for queues of
 * waiting and ready to resume coroutines. Instances of resumable_item_t
 * will be created on the stack in coroutines and then bound in
 * intrusive queues.
 *
 * @attention
 * Object of this type holds a coroutine_handle but doesn't own it.
 * The m_to_be_resumed won't be destroyed in the destructor of
 * resumable_item_t.
 *
 * @note
 * This type doesn't support Copy and Move operations.
 *
 * @since v.5.8.7
 */
class SO_5_TYPE resumable_item_t
	{
	public:
		/// @brief Enumeration of possible statuses for the coroutine
		/// for that the resumable_item is created.
		///
		/// @since v.5.8.7
		enum class status_t
			{
				/*!
				 * There is no explicit status for the coroutine.
				 *
				 * This is the initial status for a coroutine.
				 * It is also the status of the currently running coroutine.
				 *
				 * When coroutine has this status it shouldn't be resumed
				 * (because it's running now), but the coroutine can be
				 * suspended.
				 */
				neutral,
				/*!
				 * Coroutine received a notification to be resumed, but isn't
				 * in waiting/ready list of the scheduler.
				 *
				 * It is the status for currently running coroutine that
				 * receives another notification for resumption. But the
				 * resumption isn't necessary because the coroutine is
				 * running now. When a coroutine has this status it can't be
				 * suspended.
				 */
				notified,
				/*!
				 * Coroutine is in waiting list of the scheduler.
				 */
				waiting,
				/*!
				 * Coroutine is in ready list of the scheduler.
				 */
				ready
			};
		static_assert( std::atomic< status_t >::is_always_lock_free );

		//! Part of stuff for deferred initialization.
		enum class deffered_initialization_t { deffered };

		//! A special indicator for deffered initialization.
		static constexpr deffered_initialization_t deffered_init =
				deffered_initialization_t::deffered;

		/// @brief Holder of the information needed for the scheduler.
		///
		/// @note
		/// This type doesn't support Copy and Move operations.
		///
		/// @since v.5.8.7
		struct scheduler_data_t
			{
				/*!
				 * @brief Special wrapper to hold scheduler-specific
				 * additional data for resumable_item_t.
				 *
				 * It's somewhat similar to std::any, but doesn't support
				 * copy and requires explicit deleter for the value.
				 *
				 * @since v.5.8.7
				 */
				class extra_data_t
					{
					public:
						using deleter_t = void(*)(void *) noexcept;

					private:
						//! Pointer to the extra data.
						//!
						//! @note
						//! nullptr means that there is no extra data (maybe as
						//! the result of move operation).
						void * m_data{ nullptr };

						//! Pointer to the deleter of the extra data.
						//!
						//! @note
						//! nullptr means that there is no extra data (maybe as
						//! the result of move operation).
						deleter_t m_deleter{ nullptr };

					public:
						//! Default constructor.
						extra_data_t() noexcept = default;

						//! Initializing constructor.
						extra_data_t(
							void * data,
							deleter_t deleter ) noexcept
							: m_data{ data }
							, m_deleter{ deleter }
							{}

						//! Move constructor.
						extra_data_t( extra_data_t && other ) noexcept
							: m_data{ std::exchange( other.m_data, nullptr ) }
							, m_deleter{ std::exchange( other.m_deleter, nullptr ) }
							{}

						// Copy is disabled.
						extra_data_t( const extra_data_t & ) = delete;

						~extra_data_t() noexcept
							{
								if( m_deleter )
									m_deleter( m_data );
							}

						friend void
						swap( extra_data_t & a, extra_data_t & b ) noexcept
							{
								std::swap( a.m_data, b.m_data );
								std::swap( a.m_deleter, b.m_deleter );
							}

						//! Move operator.
						extra_data_t &
						operator=( extra_data_t && other ) noexcept
							{
								extra_data_t tmp{ std::move(other) };
								swap( *this, tmp );
								return *this;
							}

						//! Check the presence of the value.
						[[nodiscard]]
						bool
						has_value() const noexcept
							{
								return nullptr != m_data;
							}

						//! Get pointer to the value.
						//!
						//! Returns nullptr if there is no value.
						template< typename T >
						[[nodiscard]]
						T *
						as_ptr() const noexcept
							{
								return static_cast<T *>(m_data);
							}

						//! Get the value.
						//!
						//! @attention
						//! Doesn't check m_data for nullptr. So it's unsafe to
						//! use this method if has_value() returns `false`.
						template< typename T >
						[[nodiscard]]
						T &
						as_ref() const noexcept
							{
								return *(this->as_ptr<T>());
							}

						//! Get pointer to the value with calling a factory
						//! if the value is absent.
						//!
						//! The @a factory has to return extra_data_t instance.
						template< typename T, typename Factory >
						[[nodiscard]]
						T &
						get_or_create( Factory && factory )
							{
								if( !this->has_value() )
									*this = factory();

								return this->as_ref<T>();
							}
					};

				/// What need to be resumed.
				std::coroutine_handle<> m_to_be_resumed;

				/// The previous item in a queue.
				resumable_item_t * m_prev{ nullptr };

				/// The next item in a queue.
				resumable_item_t * m_next{ nullptr };

				/// The current status of the coroutine.
				///
				/// @note
				/// It's atomic to make possible checking of the status
				/// without acquiring the scheduler lock.
				std::atomic< status_t > m_status{ status_t::neutral };

				/// Remaining time for sleeping.
				///
				/// @note
				/// Has value only if sleeping time was specified, otherwise it's
				/// empty.
				std::optional< so_5::details::remaining_time_counter_t >
					m_sleeping_time;

				/// @brief Extra data for the scheduler.
				///
				/// If a scheduler need some additional information about
				/// the coroutiner it has to create an appropriate data object
				/// and store this object here.
				extra_data_t m_extra_data;

				/// @brief Initializing constructor.
				explicit scheduler_data_t(
					/// Handle of the coroutine to be resumed.
					std::coroutine_handle<> to_be_resumed )
					: m_to_be_resumed{ to_be_resumed }
					{}

				/// @brief Constructor for the case of deffered initialization.
				explicit scheduler_data_t( deffered_initialization_t )
					{}

				// Copy and Move operations are disabled.
				scheduler_data_t( const scheduler_data_t & ) = delete;
				scheduler_data_t &
				operator=( const scheduler_data_t & ) = delete;

				scheduler_data_t( scheduler_data_t && ) = delete;
				scheduler_data_t &
				operator=( scheduler_data_t && ) = delete;
			};

	private:
		/// Data to be managed by the scheduler.
		scheduler_data_t m_scheduler_data;

		/// @brief Scheduler to that coroutine is bound.
		///
		/// @attention
		/// May be nullptr in the case of deffered initialization.
		scheduler_t * m_scheduler;

	public:
		/// @brief Access to the scheduler's data of this resumable_item.
		///
		/// A resumable_item holds some data needed for schedulers.
		/// This data is not used by the resumable_item itself, but is
		/// stored inside the resumable_item. This method provides an
		/// access to that information.
		///
		/// @note
		/// It's assumed that only schedulers will use this method.
		[[nodiscard]]
		scheduler_data_t &
		scheduler_data() { return m_scheduler_data; }

		/// @brief Initializing constructor.
		resumable_item_t(
			/// Handle of the coroutine.
			std::coroutine_handle<> to_be_resumed,
			/// Scheduler to that coroutine has to be bound.
			scheduler_t & scheduler );

		/// @brief A special constructor for the case of deffered initialization.
		///
		/// @attention
		/// It's a UB if resumable_item_t will be used before a call to init().
		resumable_item_t( deffered_initialization_t );

		~resumable_item_t();

		// Copy and Move operations are disabled.
		resumable_item_t( const resumable_item_t & ) = delete;
		resumable_item_t &
		operator=( const resumable_item_t & ) = delete;

		resumable_item_t( resumable_item_t && ) = delete;
		resumable_item_t &
		operator=( resumable_item_t && ) = delete;

		/// @breif Actual initialization of the item.
		///
		/// @attention
		/// This method has to be used only if the constructor with
		/// deffered_initialization_t was used for resumable_item_t.
		///
		/// @attention
		/// This method may be called only once.
		void
		init(
			/// Handle of the coroutine.
			std::coroutine_handle<> to_be_resumed,
			/// Scheduler to that coroutine has to be bound.
			scheduler_t & scheduler,
			/// Initial state of the coroutine.
			status_t initial_status );

		/// @brief An attempt to resume the coroutine.
		///
		/// Delegates the work to `m_scheduler.try_schedule(*this)`.
		void
		try_schedule();

		/// @brief An attempt to suspend the coroutine for the specified
		/// amount of time.
		///
		/// Delegates the work to `m_scheduler.try_suspend(*this, sleep_time)`.
		[[nodiscard]]
		try_suspend_result_t
		try_suspend(
			monotonic_clock_t::duration sleep_time );
	};

//
// scheduler_t
//
/*!
 * @brief Interface of a coroutine scheduler.
 *
 * All implementations of coroutine schedulers have to implement
 * this interface.
 *
 * @since v.5.8.7
 */
class SO_5_TYPE scheduler_t
	{
	public:
		scheduler_t();
		virtual ~scheduler_t();

//FIXME: document why this method is not marked as noexcept.
//FIXME: is this method still needed? It seems that it is not used now
//in coroutine-related features. But it is still here until completion
//of v.5.8.7 because there is probability that this method may be
//necessary somewhere. If not then it has to be removed before the
//release of v.5.8.7.
		/// Access to SObjectizer Environment for that this scheduler has
		/// been created.
		///
		/// @note
		/// This method may throw if the scheduler doesn't bound to an
		/// environment.
		[[nodiscard]] virtual
		so_5::environment_t &
		environment() const = 0;

		/*!
		 * @brief An attempt to schedule a coroutine for resumption.
		 *
		 * If @a what_to_resume is not running now it has to be placed into
		 * a queue of ready-to-resume coroutines and should be resumed later
		 * at some moment of time.
		 *
		 * @attention
		 * The @a what_to_resume has to be valid reference and it should
		 * remain valid until the coroutine will be scheduled.
		 */
		virtual
		void
		try_schedule(
			/// What need to be resumed.
			resumable_item_t & what_to_resume ) = 0;

		/*!
		 * @brief An attempt to suspend the currently running coroutine.
		 *
		 * @note
		 * This attempt may success if the coroutine has no pending notifications
		 * that require resumption. But it also may fail in case if the coroutine
		 * has a notification for resumption. In that case try_suspend
		 * returns try_suspend_result_t::should_be_resumed and the current
		 * coroutine should continue its work without suspension.
		 *
		 * @attention
		 * The @a what_to_resume has to be valid reference and it should
		 * remain valid until the coroutine will be scheduled.
		 */
		[[nodiscard]] virtual
		try_suspend_result_t
		try_suspend(
			/// Coroutine to be processed.
			resumable_item_t & what_to_handle,
			/// How much time the coroutine could be suspended.
			///
			/// It may has infinite_speep_time() value.
			monotonic_clock_t::duration sleep_time ) = 0;
	};

//
// current_scheduler_setter_t
//
//FIXME: document this!
/*!
 * \since v.5.8.7
 */
class SO_5_TYPE current_scheduler_setter_t
	{
	public:
		//! Initializing constructor.
		current_scheduler_setter_t(
			//! The scheduler to be set as the current.
			scheduler_t & scheduler );
		~current_scheduler_setter_t();

		current_scheduler_setter_t(
			const current_scheduler_setter_t & ) = delete;
		current_scheduler_setter_t &
		operator=(
			const current_scheduler_setter_t & ) = delete;

		current_scheduler_setter_t(
			current_scheduler_setter_t && ) = delete;
		current_scheduler_setter_t &
		operator=(
			current_scheduler_setter_t && ) = delete;

		//! Get the current scheduler.
		/*!
		 * Throws an exception if there is no current scheduler.
		 */
		[[nodiscard]] static
		scheduler_t &
		safe_get();
	};

//
// scoped_scheduler_setter_t
//
//FIXME: document this!
/*!
 * \since v.5.8.7
 */
class SO_5_TYPE scoped_scheduler_setter_t
	{
		//! Previous scheduler.
		/*!
		 * Maybe nullptr.
		 */
		scheduler_t * m_previous_scheduler;

	public:
		//! Initializing constructor.
		scoped_scheduler_setter_t(
			//! The scheduler to be set as the current.
			scheduler_t & scheduler ) noexcept;
		~scoped_scheduler_setter_t();

		scoped_scheduler_setter_t(
			const scoped_scheduler_setter_t & ) = delete;
		scoped_scheduler_setter_t &
		operator=(
			const scoped_scheduler_setter_t & ) = delete;

		scoped_scheduler_setter_t(
			scoped_scheduler_setter_t && ) = delete;
		scoped_scheduler_setter_t &
		operator=(
			scoped_scheduler_setter_t && ) = delete;
	};

namespace details
{

/*!
 * @brief Helper object for suspension attempt for a coroutine.
 *
 * It's implementation detail of so_5::cpp_coro::try_suspend_for().
 *
 * @since v.5.8.7
 */
struct suspension_awaitable_t
	{
		/// Coroutine to be suspended.
		resumable_item_t & m_what_to_suspend;

		/// How much should it sleep.
		///
		/// It may have infinite_speep_time() value.
		monotonic_clock_t::duration m_sleep_time;

		[[nodiscard]]
		bool
		await_ready()
			{
				// The coroutine is ready if it's already notified.
				// If it's in neutral state then it has to be suspended.
				// Other statuses are not expected here.
				return resumable_item_t::status_t::notified ==
						m_what_to_suspend.scheduler_data().m_status.load(
								std::memory_order_acquire );
			}

		[[nodiscard]]
		bool
		await_suspend( std::coroutine_handle<> )
			{
				return try_suspend_result_t::should_be_suspended
						== m_what_to_suspend.try_suspend( m_sleep_time );
			}

		void
		await_resume() noexcept {}
	};

} /* namespace details */

/*!
 * @brief Helper function for suspension of the current coroutine if
 * coroutine has to wait for some external event.
 *
 * It was added as part of async select implementation.
 *
 * Usage example:
 * @code
 * co_await try_suspend_for(this_coroutine_resumable_item, 120ms);
 * @endcode
 *
 * The @a sleep_time can have a zero value. This doesn't mean that the
 * coroutine will not be suspended at all. Instead, it may be placed in the
 * waiting coroutines queue and suspended for a period of time until that queue
 * is processed.
 *
 * @note
 * The @a sleep_time could have infinite_speep_time() value. It may lead
 * to infinite suspension if the external event won't happen.
 *
 * @since v.5.8.7
 */
[[nodiscard]] inline
details::suspension_awaitable_t
try_suspend_for(
	/// Coroutine to be suspended.
	resumable_item_t & what_to_suspend,
	/// How much time should it sleep.
	monotonic_clock_t::duration sleep_time )
	{
		return {
				.m_what_to_suspend = what_to_suspend,
				.m_sleep_time = sleep_time
			};
	}

} /* namespace so_5::cpp_coro */

#endif


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

#include <chrono>
#include <coroutine>
#include <optional>

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
//FIXME: document this!
/// @attention
/// Object of this type holds a coroutine_handle but doesn't own it.
/// The m_to_be_resumed won't be destroyed in the destructor of
/// resumable_item_t.
///
/// @since v.5.8.7
class SO_5_TYPE resumable_item_t
	{
	public:
		/// @brief Enumeration of possible statuses for the coroutine
		/// for that the resumable_item is created.
		///
		/// @since v.5.8.7
		enum class status_t
			{
				/// There is no explicit status for the coroutine.
				neutral,
				/// Coroutine received a notification to be resumed, but isn't
				/// in waiting/ready list of the scheduler.
				notified,
				/// Coroutine is in waiting list of the scheduler.
				waiting,
				/// Coroutine is in ready list of the scheduler.
				ready
			};

		/// @brief Holder of the information needed for the scheduler.
		///
		/// @since v.5.8.7
		struct scheduler_data_t
			{
				/// What need to be resumed.
				std::coroutine_handle<> m_to_be_resumed;

				/// The previous item in a queue.
				resumable_item_t * m_prev{ nullptr };

				/// The next item in a queue.
				resumable_item_t * m_next{ nullptr };

				/// The current status of the coroutine.
				status_t m_status{ status_t::neutral };

				/// A time point for resumption.
				///
				/// May not be defined.
				std::optional< monotonic_clock_t::time_point > m_resume_at;

				/// Initializing constructor.
				explicit scheduler_data_t(
					/// Handle of the coroutine to be resumed.
					std::coroutine_handle<> to_be_resumed )
					: m_to_be_resumed{ to_be_resumed }
					{}
			};

	private:
		/// Data to be managed by the scheduler.
		scheduler_data_t m_scheduler_data;

		/// Scheduler to that coroutine is bound.
		scheduler_t & m_scheduler;

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
			/// Scheduled to that coroutine has to be bound.
			scheduler_t & scheduler );

		~resumable_item_t();

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
//FIXME: document this!
class SO_5_TYPE scheduler_t
	{
	public:

	protected:
		/// Helper for accessing coroutine's internal data.
		[[nodiscard]] static
		resumable_item_t::scheduler_data_t &
		scheduler_data_of(
			/// Coroutine to be accessed.
			resumable_item_t & what )
		noexcept;

	public:
		scheduler_t();
		virtual ~scheduler_t();

//FIXME: document why this method is not marked as noexcept.
//FIXME: is this method still needed?
		/// Access to SObjectizer Environment for that this scheduler has
		/// been created.
		[[nodiscard]] virtual
		so_5::environment_t &
		environment() const = 0;

		//FIXME: document this!
		/// @attention
		/// The @a what_to_resume has to be valid reference and it should
		/// remain valid until the coroutine will be scheduled.
		virtual
		void
		try_schedule(
			/// What need to be resumed.
			resumable_item_t & what_to_resume ) = 0;

		//FIXME: document this!
		//
		/// @attention
		/// The @a what_to_resume has to be valid reference and it should
		/// remain valid until the coroutine will be scheduled.
		virtual
		try_suspend_result_t
		try_suspend(
			/// Coroutine to be processed.
			resumable_item_t & what_to_handle,
			/// How much time the coroutine could be suspended.
			///
			/// It may has infinite_speep_time() value.
			monotonic_clock_t::duration sleep_time ) = 0;
	};

//FIXME: maybe it's not a good name.
//FIXME: document this!
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
				//FIXME: is it good to return `false` always? Maybe it's better
				//to check the status of the coroutine and return `false` only
				//if the coroutine in the neutral status?
				return false;
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

//FIXME: document this!
[[nodiscard]] inline
suspension_awaitable_t
make_awaitable_for(
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


/*
 * SObjectizer-5
 */

/*!
 * @file
 * @brief Special event_queue for case when agent should support
 * C++20 coroutines.
 *
 * @since v.5.8.7
 */

#if !defined( SO_5_IMPL_CPP_CORO_CASE_SPECIAL_QUEUE_HPP )
#define SO_5_IMPL_CPP_CORO_CASE_SPECIAL_QUEUE_HPP

#include <so_5/execution_demand.hpp>
#include <so_5/event_queue.hpp>

#include <so_5/details/invoke_noexcept_code.hpp>

#include <mutex>
#include <optional>

namespace so_5::impl
{

//
// msg_wakeup_agent_for_cpp_coro_case
//
/*!
 * \brief Special signal for case when agent support C++20 coroutines.
 *
 * \note
 * This signal isn't sent, but its type_index is used in the special
 * wakeup_demand.
 *
 * \since v.5.8.7
 */
struct msg_wakeup_agent_for_cpp_coro_case final
	: public so_5::signal_t
	{};

//
// cpp_coro_case_special_queue_t
//
//FIXME: document this!
/*!
 * \since v.5.8.7
 */
class cpp_coro_case_special_queue_t final
	: public event_queue_t
	{
	public:
		/*!
		 * \brief A single execution demand to be stored in the queue.
		 *
		 * \since v.5.8.7
		 */
		struct demand_t
			{
				//! Execution demand to be used.
				/*!
				 * \note
				 * It may be empty (if the default constructor was used for
				 * preallocated evt_start/evt_finish demands).
				 */
				execution_demand_t m_execution_demand;

				//! Next demand in the queue.
				/*!
				 * \note
				 * It's a dynamically allocated object that has to be deallocated
				 * manually during the destruction of the queue.
				 */
				demand_t * m_next = nullptr;

				/*!
				 * \brief Default constructor.
				 *
				 * It's necessary for preallocation of evt_start and evt_finish
				 * demands.
				 */
				demand_t() = default;

				//! Initializing constructor.
				demand_t( execution_demand_t && source )
					:	m_execution_demand{ std::move( source ) }
					{}
			};

		/*!
		 * \brief An alias for unique_ptr to demand.
		 *
		 * \since v.5.8.7
		 */
		using demand_unique_ptr_t = std::unique_ptr< demand_t >;

		//FIXME: document this!
		enum class current_coroutine_presence_t
			{
				absent,
				present
			};

		static constexpr current_coroutine_presence_t no_current_coro =
				current_coroutine_presence_t::absent;

		static constexpr current_coroutine_presence_t current_coro_exists =
				current_coroutine_presence_t::present;

		//FIXME: document this!
		enum class sequential_limit_status_t
			{
				not_exceeded_yet,
				exceeded
			};

		//FIXME: document this!
		[[nodiscard]] static
		sequential_limit_status_t
		check_sequential_limit(
			//! The current number of sequentially processed demands.
			unsigned int current_number,
			//! The limit.
			unsigned int limit_value ) noexcept
			{
				return current_number < limit_value
						? sequential_limit_status_t::not_exceeded_yet
						: sequential_limit_status_t::exceeded
						;
			}

	private:
		//FIXME: the std::mutex is used just for the simplicity of the first
		//implementation. It has to be changed to the appropriate
		//queue_traits::lock_unique_ptr_t.
		//! Queue lock.
		/*!
		 * \note
		 * It's an external lock-object that will be shared between several
		 * entities (like this special_queue, this_agent_scheduler and so on).
		 */
		std::mutex & m_lock;

		//! Actual event_queue for passing demands to the dispatcher.
		/*!
		 * \attention
		 * Will receive the value in bind_actual_queue method.
		 */
		event_queue_t * m_actual_queue{};

		//! Head of the queue.
		/*! Null if queue is empty. */
		demand_t * m_head = nullptr;

		//! Tail of the queue.
		/*! Null if queue is empty. */
		demand_t * m_tail = nullptr;

		//! Buffer for pending evt_finish.
		/*!
		 * The evt_finish demand will be stored here for some time if
		 * there is no possibility to push it to the actual_queue in
		 * push_evt_finish (for example, if the current coroutine
		 * exists).
		 */
		std::optional< execution_demand_t > m_pending_evt_finish;

		//FIXME: document this!
		bool m_normal_work_enabled{ false };

		//! Status of the wakeup_demand.
		/*!
		 * Value `true` means that the queue is empty and there is no
		 * current coroutine, therefore the first incoming demand
		 * has to initiate the wakeup_demand.
		 *
		 * Value `false` means that the processing of non-empty queue
		 * is not completed yet (some demands may still be in the queue
		 * or there is a suspended current top-level coroutine).
		 *
		 * There may be case when the queue is empty, but the flag still
		 * has `false` value. For example, when the last demand from the
		 * queue is extracted, but not processed yet. Or if the last
		 * extracted demand has created a new top-level coroutine that
		 * is suspended for now.
		 *
		 * The value will be changed from `false` to `true` only in
		 * try_extract_demand_for_execution() call that is performed
		 * on empty queue.
		 */
		bool m_wakeup_demand_enabled{ true };

		//! Helper method for deleting queue's head object.
		demand_unique_ptr_t
		remove_head() noexcept;

		//! Helper method for storing the demand to the tail of the queue.
		/*!
		 * \note
		 * This method doesn't change m_wakeup_demand_enabled flag.
		 */
		void
		store_demand( demand_unique_ptr_t tail_demand );

		//! Helper for pushing the wakeup_demand to the actual queue.
		/*!
		 * \attention
		 * This method is noexcept because there is no way to restore the
		 * normal state if attempt of pushing wakeup_demand fails.
		 */
		void
		push_wakeup_demand_to_actual_queue(
			agent_t * agent_to_wakeup ) noexcept;

	public:
		//! Initializing constructor.
		cpp_coro_case_special_queue_t(
			//! Lock to be used for thread safety.
			std::mutex & lock );

		~cpp_coro_case_special_queue_t() noexcept override;

		//! Connect this special_queue with the actual event_queue from
		//! the real dispatcher.
		void
		bind_actual_queue( event_queue_t * actual_queue );

		//FIXME: document this!
		void
		turn_to_normal_working_mode(
			current_coroutine_presence_t current_coroutine_presence ) noexcept;

		void
		push( execution_demand_t src_demand ) override;

		void
		push_evt_start( execution_demand_t demand ) override;

		void
		push_evt_finish( execution_demand_t demand ) noexcept override;

		//FIXME: document this!
		/*!
		 * \attention
		 * This method must be called only when m_lock is acquired by the
		 * current thread.
		 *
		 * \note
		 * This method is marked as noexcept because if an attempt to push
		 * execution_demand to the queue fails then it's impossible to
		 * resume the current top-level coroutine and normal work can't
		 * be continued.
		 */
		void
		push_coro_resumption_demand( execution_demand_t demand ) noexcept;

		//FIXME: document this!
		[[nodiscard]]
		demand_unique_ptr_t
		try_extract_demand_for_execution(
			sequential_limit_status_t limit_status );

		//! Give away the pointer to the actual_queue.
		/*!
		 * \attention
		 * The special queue shouldn't be used after this call.
		 */
		[[nodiscard]]
		event_queue_t *
		giveaway_actual_queue() noexcept;
	};

} /* namespace so_5::impl */

#endif


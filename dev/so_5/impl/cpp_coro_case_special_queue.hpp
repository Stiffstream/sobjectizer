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

		//! Status of the wakeup_demand.
		/*!
		 * Value `true` means that the queue is empty and there is no
		 * current coroutine, therefore the first incoming demand
		 * has to initiate the wakeup_demand.
		 *
		 * Value `false` means that the queue is not empty or there is
		 * the current coroutine. If there is no current coroutine, but
		 * the value is `false` then the wakeup_demand is already sent.
		 */
		bool m_wakeup_demand_enabled{ true };

		//! Helper method for deleting queue's head object.
		demand_unique_ptr_t
		remove_head() noexcept;

		void
		store_demand( demand_unique_ptr_t tail_demand );

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

		void
		push( execution_demand_t src_demand ) override;

		void
		push_evt_start( execution_demand_t demand ) override;

		void
		push_evt_finish( execution_demand_t demand ) noexcept override;

		//FIXME: document this!
		//FIXME: should this method be noexcept? It seems that the normal work
		//can't be continued if an exception will be thrown here and the demand
		//won't be stored in the agent's queue.
		void
		push_coro_resumption_demand( execution_demand_t demand );

		//FIXME: document this!
		[[nodiscard]]
		demand_unique_ptr_t
		try_extract_demand_for_execution();

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


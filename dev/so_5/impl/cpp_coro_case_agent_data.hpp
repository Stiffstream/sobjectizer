/*
 * SObjectizer-5
 */

/*!
 * @file
 * @brief Data with information related to handling C++20 coroutines
 * for an agent.
 *
 * @since v.5.8.7
 */

#if !defined( SO_5_IMPL_CPP_CORO_CASE_AGENT_DATA_HPP )
#define SO_5_IMPL_CPP_CORO_CASE_AGENT_DATA_HPP

#include <so_5/impl/cpp_coro_case_special_queue.hpp>
#include <so_5/impl/this_agent_scheduler.hpp>

#include <so_5/cpp_coro/details/top_level_task.hpp>

#include <memory>
#include <optional>

namespace so_5::impl
{

//
// cpp_coro_completion_watcher_t
//
//FIXME: document this!
/*!
 * @since v.5.8.7
 */
class cpp_coro_completion_watcher_t
	{
	public:
		virtual ~cpp_coro_completion_watcher_t() = default;

		//! This method will be called when the current top-level
		//! task is completed.
		virtual
		void
		on_completion() noexcept = 0;
	};

//! An alias for unique_ptr to cpp_coro_completion_watcher_t.
/*!
 * \since v.5.8.7
 */
using cpp_coro_completion_watcher_uptr_t =
		std::unique_ptr< cpp_coro_completion_watcher_t >;

//
// cpp_coro_case_agent_data_t
//
/*!
 * @brief Data related to C++20 coroutines support for an agent.
 *
 * @note
 * It's assumed that instance of this type will be created only
 * when C++20 coroutines support is enabled for an agent.
 *
 * @since v.5.8.7
 */
struct cpp_coro_case_agent_data_t
	{
		//! Short alias for top_level_task.
		using top_level_task_t =
				so_5::cpp_coro::details::top_level_task_t< void >;

		//FIXME: the std::mutex is used just for the simplicity of the first
		//implementation. It has to be changed to the appropriate
		//queue_traits::lock_unique_ptr_t.
		//! Lock to be used for thread safety.
		std::mutex m_lock;

		//! Special event queue for the agent.
		cpp_coro_case_special_queue_t m_special_queue;

		//! Implementation of coroutines scheduler for this agent.
		this_agent_scheduler_t m_this_agent_scheduler;

		//! An optional watcher for coroutine completion.
		/*!
		 * By default it is nullptr. But it can be set to an actual watcher.
		 * For example, testing facility uses this field.
		 *
		 * \note
		 * This object will be destroyed after completion of the
		 * current top-level task.
		 */
		cpp_coro_completion_watcher_uptr_t m_current_completion_watcher;

		//! The current coroutine.
		/*!
		 * \note
		 * This field has a value only when coroutine is started but
		 * not done yet.
		 */
		std::optional< top_level_task_t > m_current_top_level_task;

		//! How many demands may be processed sequentially.
		unsigned int m_sequential_limit;

		//! The initializing constructor.
		cpp_coro_case_agent_data_t(
			//! The target agent.
			partially_constructed_agent_ptr_t receiver,
			//! Environment for the agent.
			environment_t & env,
			//! The limit for the local queue size.
			std::size_t queue_size_limit,
			//! How many demands may be processed sequentially.
			unsigned int sequential_limit )
			: m_special_queue{ env, queue_size_limit, m_lock }
			, m_this_agent_scheduler{ *(receiver.ptr()), m_lock, m_special_queue }
			, m_sequential_limit{ sequential_limit }
			{}
	};

} /* namespace so_5::impl */

#endif


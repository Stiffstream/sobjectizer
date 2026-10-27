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

#include <optional>

namespace so_5::impl
{

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

		//! The current coroutine.
		/*!
		 * \note
		 * This field has a value only when coroutine is started but
		 * not done yet.
		 */
		std::optional< top_level_task_t > m_current_top_level_task;

		//! The initializing constructor.
		cpp_coro_case_agent_data_t( so_5::agent_t & receiver )
			: m_special_queue{ m_lock }
			, m_this_agent_scheduler{ receiver, m_lock, m_special_queue }
			{}
	};

} /* namespace so_5::impl */

#endif


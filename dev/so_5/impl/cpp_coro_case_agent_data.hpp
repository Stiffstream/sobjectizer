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
		//! Special event queue for the agent.
		cpp_coro_case_special_queue_t m_special_queue;

		//FIXME: the current coroutine has to be stored here.
	};

} /* namespace so_5::impl */

#endif


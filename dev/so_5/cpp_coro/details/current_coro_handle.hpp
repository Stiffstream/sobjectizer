/*
 * SObjectizer-5
 */

/// @file
/// @brief Helpers to obtain coroutine_handle for the current coroutine.
///
/// @since v.5.8.7

#if !defined(SO_5_CPP_CORO_DETAILS_CURRENT_CORO_HANDLE_HPP)
#define SO_5_CPP_CORO_DETAILS_CURRENT_CORO_HANDLE_HPP

#include <coroutine>

namespace so_5::cpp_coro::details
{

//FIXME: document this!
struct current_coro_handle_obtainer_t
	{
		std::coroutine_handle<> m_handle;

		bool
		await_ready() const noexcept { return false; }

		bool
		await_suspend( std::coroutine_handle<> handle ) noexcept
			{
				m_handle = handle;
				return false;
			}

		std::coroutine_handle<>
		await_resume() const noexcept
			{
				return m_handle;
			}
	};

//FIXME: document this!
[[nodiscard]] inline
current_coro_handle_obtainer_t
current_coro_handle()
	{
		return {};
	}

} /* namespace so_5::cpp_coro::details */

#endif


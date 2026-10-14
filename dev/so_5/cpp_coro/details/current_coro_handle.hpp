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

namespace current_coro_handle_impl
{

/// Special awaitable type that allows to get coroutine handle.
///
/// Stores the handle passed to await_suspend() and returns it
/// in await_resume().
///
/// @note
/// await_suspend() return `false` (doesn't block the caller).
///
/// @since v.5.8.7
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

} /* namespace current_coro_handle_impl */

/// Helper for obtaining handle of the current coroutine.
///
/// Usage example:
/// @code
/// so_5::cpp_coro::task_t<void>
/// some_coroutine()
/// {
/// 	const auto this_coro_handle =
/// 		co_await so_5::cpp_coro::details::current_coro_handle();
/// 	...
/// }
/// @endcode
///
/// @since v.5.8.7
[[nodiscard]] inline
auto
current_coro_handle()
	{
		return current_coro_handle_impl::current_coro_handle_obtainer_t{};
	}

} /* namespace so_5::cpp_coro::details */

#endif


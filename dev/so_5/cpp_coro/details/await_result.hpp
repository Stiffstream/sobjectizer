/*
 * SObjectizer-5
 */

/// @file
/// @brief Implementation of await_result template.
///
/// @since v.5.8.6

#if !defined( SO_5_CPP_CORO_DETAILS_AWAIT_RESULT_HPP )
#define SO_5_CPP_CORO_DETAILS_AWAIT_RESULT_HPP

#include <concepts>
#include <coroutine>
#include <type_traits>

// NOTE: the following implementation is produced from the code
// from https://wg21.link/P1288R0 proposal.

namespace so_5::cpp_coro::details
{

template<typename Tp>
struct is_valid_await_suspend_return_type : std::false_type {};

template<>
struct is_valid_await_suspend_return_type<bool> : std::true_type {};

template<>
struct is_valid_await_suspend_return_type<void> : std::true_type {};

template<typename Promise>
struct is_valid_await_suspend_return_type< std::coroutine_handle<Promise> >
	: std::true_type {};

template<typename Tp>
concept Await_suspend_return_type = is_valid_await_suspend_return_type<Tp>::value;

template<typename Tp>
concept Awaiter =
requires(Tp && awaiter, std::coroutine_handle<void> h)
	{
		// await_ready() result must be contextually convertible to bool.
		{ awaiter.await_ready() } -> std::convertible_to<bool>;

		awaiter.await_suspend(h);

		requires Await_suspend_return_type<decltype(awaiter.await_suspend(h))>;

		awaiter.await_resume();
	};

template<typename Tp>
concept Weak_has_member_co_await =
requires(Tp && awaitable)
	{
		static_cast<Tp &&>(awaitable).operator co_await();
	};

template<typename Tp>
concept Weak_has_non_member_co_await =
	requires(Tp && awaitable)
	{
		operator co_await(static_cast<Tp &&>(awaitable));
	};

template<Weak_has_member_co_await Tp>
decltype(auto)
get_awaiter(Tp && awaitable)
noexcept(noexcept(static_cast<Tp&&>(awaitable).operator co_await()))
	{
		return static_cast<Tp&&>(awaitable).operator co_await();
	}

template<Weak_has_non_member_co_await Tp>
decltype(auto)
get_awaiter(Tp && awaitable)
noexcept(noexcept(operator co_await(static_cast<Tp&&>(awaitable))))
	{
		return operator co_await(static_cast<Tp&&>(awaitable));
	}

template<typename Tp>
requires (!Weak_has_non_member_co_await<Tp> && !Weak_has_member_co_await<Tp>)
Tp &&
get_awaiter(Tp && awaitable) noexcept
	{
		return static_cast<Tp &&>(awaitable);
	}

template<typename Tp>
struct awaiter_type
	{
		using type = decltype(get_awaiter(std::declval<Tp>()));
	};

template<typename Tp>
using awaiter_type_t = typename awaiter_type<Tp>::type;

template<typename Tp>
//FIXME: is std::movable<Tp> really needed here?
concept Awaitable = /*std::movable<Tp> &&*/ requires(Tp && awaitable)
	{
		{ get_awaiter(static_cast<Tp &&>(awaitable)) } -> Awaiter;
	};

template<typename Tp>
struct await_result {};

template<Awaitable Tp>
struct await_result<Tp>
	{
		using type = decltype(
				std::declval<awaiter_type_t<Tp>&>().await_resume());
	};

template<typename Tp>
using await_result_t = typename await_result<Tp>::type;

} /* namespace so_5::cpp_coro::details */

#endif


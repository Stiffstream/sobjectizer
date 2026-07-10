/*
 * SObjectizer-5
 */

/// @file
/// @brief Staff related to coroutine-friendly select function.
///
/// @since v.5.8.6

#if !defined( SO_5_CPP_CORO_MCHAIN_SELECT_HPP )
#define SO_5_CPP_CORO_MCHAIN_SELECT_HPP

#include <so_5/declspec.hpp>

#include <so_5/mchain_select.hpp>

#include <so_5/cpp_coro/details/final_awaiter.hpp>

#include <exception>

namespace so_5::cpp_coro
{

//
// resumable_select_t
//
//FIXME: document this!
class SO_5_TYPE resumable_select_t
	{
	public:
		//FIXME: document this!
		struct waiting_for_next_event_t {};

		//FIXME: document this!
		class promise_type
			{
				// NOTE: has to be set in unhandled_exception.
				std::exception_ptr m_exception;

				// NOTE: has to be set by async_select_task_t::awaiter_t.
				std::coroutine_handle<> m_continuation;

				// NOTE: has to be set in return_value.
				mchain_select_result_t m_select_result;

			public:
				promise_type() = default;

				std::suspend_always
				initial_suspend() noexcept
					{
						return {};
					}

				so_5::cpp_coro::details::final_awaiter_t< promise_type >
				final_suspend() noexcept
					{
						return {};
					}

				void
				return_value( mchain_select_result_t result ) noexcept
					{
						m_select_result = result;
					}

				std::suspend_always
				yield_value( waiting_for_next_event_t ) noexcept
					{
						return {};
					}

				void
				unhandled_exception() noexcept
					{
						m_exception = std::current_exception();
					}

				[[nodiscard]]
				resumable_select_t
				get_return_object() noexcept
					{
						return {
								std::coroutine_handle< promise_type >::from_promise(
										*this )
							};
					}

				void
				set_continuation(
					std::coroutine_handle<> continuation ) noexcept
					{
						m_continuation = continuation;
					}

				[[nodiscard]]
				std::coroutine_handle<>
				query_continuation() const noexcept
					{
						return m_continuation;
					}

				[[nodiscard]]
				mchain_select_result_t
				select_result() const
					{
						if( m_exception )
							std::rethrow_exception( m_exception );

						return m_select_result;
					}
			};

		//FIXME: document this!
		/// @attention
		/// Awaiter holds a reference to coroutine, but doesn't own it and
		/// doesn't destroy it.
		class awaiter_t
			{
				std::coroutine_handle< promise_type > m_coro;

			public:
				awaiter_t( std::coroutine_handle< promise_type > coro )
					: m_coro{ coro }
					{}

				[[nodiscard]]
				bool
				await_ready() const noexcept { return false; }

				[[nodiscard]]
				std::coroutine_handle<>
				await_suspend( std::coroutine_handle<> parent_coro ) noexcept
					{
						m_coro.promise().set_continuation( parent_coro );

						return m_coro;
					}

				[[nodiscard]]
				mchain_select_result_t
				await_resume()
					{
						return m_coro.promise().select_result();
					}
			};

		[[nodiscard]]
		awaiter_t operator co_await() noexcept
			{
				return { m_coro };
			}

		resumable_select_t( const resumable_select_t & ) = delete;
		resumable_select_t &
		operator=( const resumable_select_t & ) = delete;

		resumable_select_t( resumable_select_t && ) noexcept;
		resumable_select_t &
		operator=( resumable_select_t && ) noexcept;

		~resumable_select_t();

	private:
		std::coroutine_handle< promise_type > m_coro;

		/// Initializing constructor.
		resumable_select_t(
			std::coroutine_handle< promise_type > coro ) noexcept;

		[[nodiscard]] static
		std::coroutine_handle< promise_type >
		take_out_from( resumable_select_t & other )
		noexcept;

		static
		void
		destroy_if_needed(
			std::coroutine_handle< promise_type > coro )
		noexcept;
	};

} /* namespace so_5::cpp_coro */

#endif


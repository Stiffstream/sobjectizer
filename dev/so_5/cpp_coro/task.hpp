/*
 * SObjectizer-5
 */

/// @file
/// @brief The `task_t` template.
///
/// @since v.5.8.7

#if !defined( SO_5_CPP_CORO_TASK_HPP )
#define SO_5_CPP_CORO_TASK_HPP

#include <so_5/cpp_coro/details/final_awaiter.hpp>

#include <so_5/exception.hpp>
#include <so_5/ret_code.hpp>

#include <coroutine>
#include <variant>
#include <type_traits>

namespace so_5::cpp_coro
{

namespace task_impl
{

//FIXME: document this!
struct no_value_t {};

//FIXME: document this!
struct return_void_called_t {};

//FIXME: document this!
template< typename T >
struct value_to_hold_selector_t
	{
		using type = T;
	};

template<>
struct value_to_hold_selector_t< void >
	{
		using type = return_void_called_t;
	};

template< typename T >
using value_to_hold_t = typename value_to_hold_selector_t< T >::type;

//FIXME: document this!
template<
	typename Actual_Promise_Type,
	typename T >
struct promise_base_t
	{
		using storage_t = std::variant<
				no_value_t,
				std::exception_ptr,
				value_to_hold_t< T >
			>;

		storage_t m_result{ no_value_t{} };

		std::coroutine_handle<> m_continuation;

		std::suspend_always
		initial_suspend() noexcept { return {}; }

		so_5::cpp_coro::details::final_awaiter_t< Actual_Promise_Type >
		final_suspend() noexcept { return {}; }

		//FIXME: implement this!
		void
		unhandled_exception() noexcept {}

		[[nodiscard]]
		std::coroutine_handle<>
		query_continuation() const noexcept
			{
				return m_continuation;
			}

		auto
		extract_result()
			{
				switch( m_result.index() )
					{
					case 0:
						SO_5_THROW_EXCEPTION(
								rc_no_value_in_promise_object,
								"no value to return from "
								"task_t::await_resume" );
					break;

					case 1: std::rethrow_exception(
							std::move(std::get< std::exception_ptr >(m_result))
						);
					break;
					}

				if constexpr( !std::is_same_v< T, void > )
					return T{ std::move(std::get<T>(m_result)) };
			}
	};

//FIXME: document this!
template<
	typename Actual_Promise_Type,
	typename T >
struct promise_base_with_return_handling_t
	: public promise_base_t< Actual_Promise_Type, T >
	{
		template< typename Actual_T >
		void
		return_value( Actual_T && v )
			{
				this->m_result.template emplace<T>( std::forward<Actual_T>(v) );
			}
	};

template< typename Actual_Promise_Type >
struct promise_base_with_return_handling_t< Actual_Promise_Type, void >
	: public promise_base_t< Actual_Promise_Type, void >
	{
		void
		return_void()
			{
				this->m_result.template emplace< return_void_called_t >();
			}
	};

} /* namespace task_impl */

//
// task_t
//
//FIXME: document this!
template< typename T >
struct task_t
	{
		struct promise_type;

		std::coroutine_handle< promise_type > m_coro;

		task_t( std::coroutine_handle< promise_type > coro )
			: m_coro{ coro }
			{
			}

		~task_t()
			{
				if( m_coro )
				{
					m_coro.destroy();
				}
			}

		task_t( const task_t & ) = delete;
		task_t &
		operator=( const task_t & ) = delete;

		task_t( task_t && other ) noexcept
			: m_coro{ std::exchange(
					other.m_coro,
					std::coroutine_handle< promise_type >{} ) }
			{}

		task_t &
		operator=( task_t && other ) noexcept
			{
				if( this != std::addressof(other) )
					{
						if( m_coro ) m_coro.destroy();

						m_coro = std::exchange( other.m_coro,
								std::coroutine_handle< promise_type >{} );
					}

				return *this;
			}

		//FIXME: document this!
		struct promise_type final
			: public task_impl::promise_base_with_return_handling_t<
					promise_type, T >
			{
				task_t
				get_return_object() noexcept
					{
						return {
								std::coroutine_handle< promise_type >
										::from_promise( *this )
							};
					}
			};

		bool
		await_ready() const noexcept { return false; }

		std::coroutine_handle<>
		await_suspend( std::coroutine_handle<> continuation ) noexcept
			{
				m_coro.promise().m_continuation = continuation;
				return m_coro;
			}

		T
		await_resume()
			{
				return m_coro.promise().extract_result();
			}
	};

} /* namespace so_5::cpp_coro */

#endif


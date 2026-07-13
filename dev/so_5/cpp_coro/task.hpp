/*
 * SObjectizer-5
 */

/// @file
/// @brief The `task_t` template.
///
/// @since v.5.8.6

#if !defined( SO_5_CPP_CORO_TASK_HPP )
#define SO_5_CPP_CORO_TASK_HPP

#include <so_5/exception.hpp>
#include <so_5/ret_code.hpp>

#include <coroutine>
#include <variant>

namespace so_5::cpp_coro
{

namespace task_impl
{

//FIXME: document this!
struct no_value_t {};

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

		//FIXME: these methods has to be implemented.
		task_t( task_t && ) = delete;
		task_t &
		operator=( task_t && ) = delete;

		//FIXME: document this!
		struct promise_type
			{
				std::variant< task_impl::no_value_t, std::exception_ptr, T > m_result;

				promise_type()
					: m_result{ task_impl::no_value_t{} }
					{}

				task_t
				get_return_object() noexcept
					{
						return {
								std::coroutine_handle< promise_type >
										::from_promise( *this )
							};
					}

				std::suspend_always
				initial_suspend() noexcept { return {}; }

				std::suspend_always
				final_suspend() noexcept { return {}; }

				template< typename Actual_T >
				void
				return_value( Actual_T && v )
					{
						m_result.template emplace<T>( std::forward<Actual_T>(v) );
					}

				//FIXME: implement this!
				void
				unhandled_exception() noexcept {}
			};

		bool
		await_ready() const noexcept { return false; }

		void
		await_suspend( std::coroutine_handle<> ) noexcept
			{
				m_coro.resume();
			}

		T
		await_resume()
			{
				auto & promise = m_coro.promise();
				switch( promise.m_result.index() )
					{
					case 0:
						SO_5_THROW_EXCEPTION(
								rc_no_value_in_promise_object,
								"no value to return from "
								"task_t::await_resume" );
					break;

					case 1: std::rethrow_exception(
							std::move(std::get< std::exception_ptr >(
									promise.m_result))
						);
					break;
					}

				return T{ std::move(std::get<T>(promise.m_result)) };
			}
	};

//FIXME: document this!
template<>
struct task_t< void >
	{
		struct return_void_called_t {};

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

		//FIXME: these methods have to be implemented!
		task_t( task_t && ) = delete;
		task_t &
		operator=( task_t && ) = delete;

		//FIXME: document this!
		struct promise_type
			{
				std::variant<
						task_impl::no_value_t,
						std::exception_ptr,
						return_void_called_t >
					m_result;

				promise_type()
					: m_result{ task_impl::no_value_t{} }
					{}

				task_t
				get_return_object() noexcept
					{
						return {
								std::coroutine_handle< promise_type >
										::from_promise( *this )
							};
					}

				std::suspend_always
				initial_suspend() noexcept { return {}; }

				std::suspend_always
				final_suspend() noexcept { return {}; }

				void
				return_void()
					{
						m_result.template emplace< return_void_called_t >();
					}

				//FIXME: implement this!
				void
				unhandled_exception() noexcept {}
			};

		bool
		await_ready() const noexcept { return false; }

		void
		await_suspend( std::coroutine_handle<> ) noexcept
			{
				m_coro.resume();
			}

		void
		await_resume()
			{
				auto & promise = m_coro.promise();
				switch( promise.m_result.index() )
					{
					case 0:
						SO_5_THROW_EXCEPTION(
								rc_no_value_in_promise_object,
								"no value to return from "
								"top_level_task_t::await_resume" );
					break;

					case 1: std::rethrow_exception(
							std::move(std::get< std::exception_ptr >(
									promise.m_result))
						);
					break;
					}

				// Nothing to return because it's `void`.
			}
	};

} /* namespace so_5::cpp_coro */

#endif


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
#include <exception>
#include <type_traits>
#include <utility>
#include <variant>

namespace so_5::cpp_coro
{

namespace task_impl
{

/*!
 * @brief Helper type to be used in case there is no value for a task object
 * yet.
 *
 * @since v.5.8.7
 */
struct no_value_t {};

/*!
 * @brief Helper type to be used when the return_void method is called on a
 * promise_type object.
 *
 * @since v.5.8.7
 */
struct return_void_called_t {};

/*!
 * @brief Implementation details of the metafunction to be used to detect the
 * actual type of data stored in promise_type storage.
 *
 * @since v.5.8.7
 */
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

/*!
 * @brief Metafunction to be used to detect the actual type of data stored in
 * promise_type storage.
 *
 * If @a T is `void` then return_void_called_t will be the result.
 *
 * @since v.5.8.7
 */
template< typename T >
using value_to_hold_t = typename value_to_hold_selector_t< T >::type;

/*!
 * @brief A base type to be used for implementation of specific promise_type.
 *
 * It's intended to be used in CRTP.
 *
 * @note
 * This type should be used directly. It's just a building block for
 * promise_base_t template.
 *
 * @attention
 * This template doesn't have return_value/return_void methods. Those
 * methods have to be implemented in derived types.
 *
 * @tparam Actual_Promise_Type type that is derived from the promise_base_common_t.
 *
 * @tparam T type of value for the promise.
 *
 * @since v.5.8.7
 */
template<
	typename Actual_Promise_Type,
	typename T >
struct promise_base_common_t
	{
		//! Type of storage for holding value or exception.
		using storage_t = std::variant<
				no_value_t,
				std::exception_ptr,
				value_to_hold_t< T >
			>;

		//! The value of the promise.
		storage_t m_result{ no_value_t{} };

		//! The next coroutine to be resumed.
		/*!
		 * It's a part of symmetric transfer implementation.
		 */
		std::coroutine_handle<> m_continuation;

		std::suspend_always
		initial_suspend() noexcept { return {}; }

		so_5::cpp_coro::details::final_awaiter_t< Actual_Promise_Type >
		final_suspend() noexcept { return {}; }

		void
		unhandled_exception()
		noexcept( noexcept(m_result = std::current_exception()) )
			{
std::cout << "*** unhandled_exception! ***" << std::endl;
				m_result = std::current_exception();
			}

		/*!
		 * @note
		 * This method is required by so_5::cpp_coro::details::final_awaiter_t.
		 */
		[[nodiscard]]
		std::coroutine_handle<>
		query_continuation() const noexcept
			{
				return m_continuation;
			}

		//! Get the value from the promise.
		/*!
		 * May throw an exception if unhandled_exception() was called for
		 * the promise or if the promise doesn't have the result value
		 * (no calls to return_value/return_void).
		 */
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

					case 1:
						std::rethrow_exception(
								std::move(std::get< std::exception_ptr >(m_result))
							);
					break;
					}

				if constexpr( !std::is_same_v< T, void > )
					return T{ std::move(std::get<T>(m_result)) };
			}
	};

/*!
 * @brief Actual promise_type template.
 *
 * It's intended to be used in CRTP.
 *
 * This specialization is used when T is not `void`.
 *
 * @tparam Actual_Promise_Type type that is derived from the promise_base_common_t.
 *
 * @tparam T type of value for the promise.
 *
 * @since v.5.8.7
 */
template<
	typename Actual_Promise_Type,
	typename T >
struct promise_base_t
	: public promise_base_common_t< Actual_Promise_Type, T >
	{
		template< typename Actual_T >
		void
		return_value( Actual_T && v )
			{
				this->m_result.template emplace<T>( std::forward<Actual_T>(v) );
			}
	};

/*!
 * @brief Actual promise_type template.
 *
 * This specialization is used when T is `void`.
 *
 * @tparam Actual_Promise_Type type that is derived from the promise_base_common_t.
 *
 * @since v.5.8.7
 */
template< typename Actual_Promise_Type >
struct promise_base_t< Actual_Promise_Type, void >
	: public promise_base_common_t< Actual_Promise_Type, void >
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
/*!
 * @brief A task template that implements awaitable for value of type T.
 *
 * The `T` could be `void`.
 *
 * This type supports only coroutines with lazy start -- the await_ready
 * returns `false`.
 *
 * @note
 * It's Movable, but not Copyable type.
 *
 * @since v.5.8.7
 */
template< typename T >
struct task_t
	{
		struct promise_type;

		//! The coroutine associated with the task.
		std::coroutine_handle< promise_type > m_coro;

		//! Initializing constructor.
		task_t( std::coroutine_handle< promise_type > coro )
			: m_coro{ coro }
			{
			}

		~task_t()
			{
				if( m_coro )
				{
std::cout << "*** destroying coro " << m_coro.address() << std::endl;
					m_coro.destroy();
				}
			}

		task_t( const task_t & ) = delete;
		task_t &
		operator=( const task_t & ) = delete;

		//! Move constructor.
		task_t( task_t && other ) noexcept
			: m_coro{ std::exchange(
					other.m_coro,
					std::coroutine_handle< promise_type >{} ) }
			{}

		//! Move operator.
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

		//! The actual promise_type for the task.
		struct promise_type
			: public task_impl::promise_base_t< promise_type, T >
			{
				//! Standard factory method that returns new task instance.
				task_t
				get_return_object() noexcept
					{
						return {
								std::coroutine_handle< promise_type >
										::from_promise( *this )
							};
					}
			};

		//! The standard-required method.
		//!
		//! Returns `false`.
		bool
		await_ready() const noexcept { return false; }

		//! The standard-required method.
		//!
		//! Stores @a continuation in the promise and returns the value
		//! of @a m_coro.
		std::coroutine_handle<>
		await_suspend( std::coroutine_handle<> continuation ) noexcept
			{
				m_coro.promise().m_continuation = continuation;
				return m_coro;
			}

		//! The standard-required method.
		//!
		//! Returns the value stored in the promise object or throws
		//! an exception if unhandled_exception was called for the promise.
		T
		await_resume()
			{
				return m_coro.promise().extract_result();
			}
	};

} /* namespace so_5::cpp_coro */

#endif


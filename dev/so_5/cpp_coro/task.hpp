/*
 * SObjectizer-5
 */

/// @file
/// @brief The `task_t` template.
///
/// @since v.5.8.7

#if !defined( SO_5_CPP_CORO_TASK_HPP )
#define SO_5_CPP_CORO_TASK_HPP

#include <so_5/cpp_coro/scheduler.hpp>

#include <so_5/cpp_coro/details/final_awaiter.hpp>
#include <so_5/cpp_coro/details/current_coro_handle.hpp>

#include <so_5/exception.hpp>
#include <so_5/ret_code.hpp>

#include <coroutine>
#include <exception>
#include <type_traits>
#include <utility>
#include <variant>

namespace so_5::cpp_coro
{

// Forward declaration.
template< typename T >
struct task_t;

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
std::cout << "___ rethrowing an exception" << std::endl;
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

//
// resumption_by_specific_scheduler_task_t
//
//FIXME: document this!
template< typename Source_Awaitable >
struct resumption_by_specific_scheduler_task_t
	{
		//! Promise type for this task type.
		struct promise_type
			{
				//! Scheduler to be used for resumption.
				so_5::cpp_coro::scheduler_t & m_scheduler;

				//! Awaitable for that this task has been created.
				Source_Awaitable m_source;

				//! The parent coroutine to be awoken.
				/*!
				 * \note
				 * The actual value is sent in await_suspend method.
				 */
				std::coroutine_handle<> m_coro;

				//! Resumable item to be used for resumption.
				so_5::cpp_coro::resumable_item_t m_resumable_item;

				//! Initializing constructor.
				promise_type(
					so_5::cpp_coro::scheduler_t & scheduler,
					Source_Awaitable source )
					: m_scheduler{ scheduler }
					, m_source{ source }
					, m_resumable_item{
						so_5::cpp_coro::resumable_item_t::deffered_init }
					{}

				resumption_by_specific_scheduler_task_t
				get_return_object()
					{
						using handle = std::coroutine_handle< promise_type >;
						return { handle::from_promise( *this ) };
					}

				std::suspend_always
				initial_suspend() const noexcept { return {}; }

				std::suspend_always
				final_suspend() const noexcept { return {}; }

				void
				return_void() noexcept {}

				void
				unhandled_exception()
					{
//FIXME: is it a good default behaviour?
						throw;
					}
			};

		//! The coroutine associated with this task instance.
		std::coroutine_handle< promise_type > m_coro;

		//! Initializing constructor.
		resumption_by_specific_scheduler_task_t(
			std::coroutine_handle< promise_type > coro )
			: m_coro{ coro }
			{}

		~resumption_by_specific_scheduler_task_t()
			{
				if( m_coro )
					{
						m_coro.destroy();
					}
			}

		// Copy is disabled.
		resumption_by_specific_scheduler_task_t(
			const resumption_by_specific_scheduler_task_t & ) = delete;
		resumption_by_specific_scheduler_task_t &
		operator=(
			const resumption_by_specific_scheduler_task_t & ) = delete;

		// Move operations.
		resumption_by_specific_scheduler_task_t(
			resumption_by_specific_scheduler_task_t && other ) noexcept
			: m_coro{ std::exchange(
					other.m_coro,
					std::coroutine_handle< promise_type >{} ) }
			{}

		resumption_by_specific_scheduler_task_t &
		operator=(
			resumption_by_specific_scheduler_task_t && other ) noexcept
			{
				if( this != std::addressof(other) )
					{
						if( m_coro )
							m_coro.destroy();

						m_coro = std::exchange(
								other.m_coro,
								std::coroutine_handle< promise_type >{} );
					}

				return *this;
			}

		bool
		await_ready() const
			{
				return m_coro.promise().m_source.await_ready();
			}

		auto
		await_suspend( std::coroutine_handle<> parent_coro )
			{
				auto & promise = m_coro.promise();
				promise.m_coro = parent_coro;
				promise.m_resumable_item.init(
						parent_coro,
						promise.m_scheduler,
						so_5::cpp_coro::resumable_item_t::status_t::waiting );

				//FIXME: is it a good approach if Source_Awaitable::await_suspend
				//returns std::coroutine_handle<>?
				return promise.m_source.await_suspend( m_coro );
			}

		auto
		await_resume()
			{
				return m_coro.promise().m_source.await_resume();
			}
	};

//FIXME: document this!
template< typename Source_Awaitable >
[[nodiscard]]
resumption_by_specific_scheduler_task_t< Source_Awaitable >
wrap_foreign_awaitable(
	so_5::cpp_coro::scheduler_t & scheduler,
	Source_Awaitable && awaitable )
	{
		(void) scheduler;
		(void) awaitable;

		using promise_type =
				typename resumption_by_specific_scheduler_task_t< Source_Awaitable >
						::promise_type;

		auto this_coro_handle =
				co_await so_5::cpp_coro::details::current_coro_handle();

//FIXME: document those actions!
		std::coroutine_handle< promise_type > this_coro =
				std::coroutine_handle< promise_type >::from_address(
						this_coro_handle.address() );

		this_coro.promise().m_resumable_item.try_schedule();
	}

//FIXME: document this!
template< typename T >
struct is_so5_task_t : public std::false_type {};

template< typename T >
struct is_so5_task_t< task_t<T> > : public std::true_type {};

template< typename T >
concept is_so5_task = is_so5_task_t< T >::value;

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

				//FIXME: document this!
				template< typename Awaitable >
				auto
				await_transform( Awaitable && awaitable )
					{
						using namespace task_impl;

						if constexpr( is_so5_task< Awaitable > )
							{
								return static_cast<Awaitable &&>(awaitable);
							}
						else
							{
								return wrap_foreign_awaitable(
										current_scheduler_setter_t::safe_get(),
										std::forward<Awaitable>(awaitable) );
							}
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


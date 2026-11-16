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

#include <so_5/atomic_refcounted.hpp>

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

//
// resumption_by_specific_scheduler_task_t
//
/*!
 * \brief Special task for resumption of a coroutine by using a specific
 * scheduler.
 *
 * This task is used when a foreign awaiter is created inside
 * asynchronous event/message handler. This awatable may be awoken on
 * a different execution context (that is not controlled by SObjectizer).
 * But resumption of a SO-5 related coroutine must be performed only
 * by the appropriate scheduler.
 *
 * For such scheduler-bound resumption a foreign awaiter is wrapped into
 * this task. When this task is resumed it asks the appropriate scheduler
 * to dispatch the corresponding coroutine on the right execution context.
 *
 * \note
 * This type doesn't support move operator (but it has the move constructor).
 * It's because if a task_t already has the task_data then this data can't
 * be just dropped in the `operator=`.
 *
 * \since v.5.8.7
 */
template< typename Source_Awaiter >
struct resumption_by_specific_scheduler_task_t
	{
		//! Special marker that this type is a SO-5 awaiter.
		using so5_awaitable = void;

		struct promise_type;

		//! Short alias for coroutine handle.
		using coro_handle_t = std::coroutine_handle< promise_type >;

		//! Short alias for the raw pointer to the source awaiter object.
		using source_awaiter_ptr_t = std::add_pointer_t<
				std::remove_reference_t< Source_Awaiter > >;

		//! Data to be schared between task and promise_type.
		/*!
		 * There could be cases when the promise object is destroyed
		 * on thread A before the destruction of the task object on
		 * thread B. But the task has to hold the valid task_data.
		 *
		 * Otherwise, there could be cases when the task object is
		 * destroyed in thread B before the destruction of the promise
		 * on thread A. But the promise object has to hold the
		 * valid task_data for the completion of try_schedule call.
		 *
		 * The easiest way is to create task_data_t in dynamic memory
		 * and use reference counting for control its lifetime.
		 */
		struct task_data_t : public so_5::atomic_refcounted_t
			{
				//! Scheduler to be used for resumption.
				so_5::cpp_coro::scheduler_t & m_scheduler;

				//! Awaiter for that this task has been created.
				source_awaiter_ptr_t m_source;

				//! The parent coroutine to be awoken.
				/*!
				 * \note
				 * The actual value is set in await_suspend method.
				 */
				std::coroutine_handle<> m_coro;

				//! Resumable item to be used for resumption.
				so_5::cpp_coro::resumable_item_t m_resumable_item;

				//! Initializing constructor.
				task_data_t(
					so_5::cpp_coro::scheduler_t & scheduler,
					source_awaiter_ptr_t source )
					: m_scheduler{ scheduler }
					, m_source{ source }
					, m_resumable_item{
							so_5::cpp_coro::resumable_item_t::deffered_init }
					{}

				task_data_t( const task_data_t & ) = delete;
				task_data_t &
				operator=( const task_data_t & ) = delete;

				task_data_t( task_data_t && ) = delete;
				task_data_t &
				operator=( task_data_t && ) = delete;
			};

		//! Alias for unique_ptr to task_data_t.
		using task_data_smart_ptr_t = so_5::intrusive_ptr_t< task_data_t >;

		//! Promise type for this task type.
		/*!
		 * \attention
		 * This promise_type just rethrows an exception in
		 * unhandled_exception() method.
		 */
		struct promise_type
			{
				//! Data related to the task that created this promise.
				task_data_smart_ptr_t m_task_data;

				//! Initializing constructor.
				promise_type(
					so_5::cpp_coro::scheduler_t & scheduler,
					source_awaiter_ptr_t source )
					: m_task_data{ std::make_unique< task_data_t >(
							scheduler,
							source )
						}
					{}

				resumption_by_specific_scheduler_task_t
				get_return_object()
					{
						return {
								coro_handle_t::from_promise( *this ),
								m_task_data
							};
					}

				std::suspend_always
				initial_suspend() noexcept
					{
						return {};
					}

				std::suspend_never
				final_suspend() noexcept
					{
						return {};
					}

				void
				return_void() noexcept {}

				void
				unhandled_exception()
					{
						throw;
					}
			};

		//! Coroutine associated with this task.
		/*!
		 * \note
		 * The task doesn't control the lifetime of this coroutine.
		 */
		coro_handle_t m_coro;

		//! Task data associated with this task.
		task_data_smart_ptr_t m_task_data;

		//! Initializing constructor.
		resumption_by_specific_scheduler_task_t(
			std::coroutine_handle< promise_type > coro,
			task_data_smart_ptr_t task_data ) noexcept
			: m_coro{ coro }
			, m_task_data{ std::move(task_data) }
			{}

		~resumption_by_specific_scheduler_task_t()
			{
				m_task_data.reset();
			}

		// Copy is disabled.
		resumption_by_specific_scheduler_task_t(
			const resumption_by_specific_scheduler_task_t & ) = delete;
		resumption_by_specific_scheduler_task_t &
		operator=(
			const resumption_by_specific_scheduler_task_t & ) = delete;

		// Move construction.
		resumption_by_specific_scheduler_task_t(
			resumption_by_specific_scheduler_task_t && other ) noexcept
			: m_coro{ std::exchange( other.m_coro, coro_handle_t{} ) }
			, m_task_data{
				std::exchange( other.m_task_data, task_data_smart_ptr_t{} ) }
			{}

		// Move operation is disabled.
		resumption_by_specific_scheduler_task_t &
		operator=( resumption_by_specific_scheduler_task_t && ) = delete;

		bool
		await_ready() const
			{
				return m_task_data->m_source->await_ready();
			}

		auto
		await_suspend( std::coroutine_handle<> parent_coro )
			{
				// The main trick: our parent coroutine to be suspended and
				// resumed later will be stored in the task_data.
				m_task_data->m_coro = parent_coro;
				m_task_data->m_resumable_item.init(
						parent_coro,
						m_task_data->m_scheduler,
						so_5::cpp_coro::resumable_item_t::status_t::waiting );

				// But the for the source awaiter our coroutine will be
				// used as the parent. When the foreign coroutine will be
				// resumed it will also resume this task.
				return m_task_data->m_source->await_suspend( m_coro );
			}

		decltype(auto)
		await_resume()
			{
				return m_task_data->m_source->await_resume();
			}
	};

/*!
 * \brief Helper function for wrapping foreign awaiter object into
 * a special task.
 *
 * This is a special coroutine that plays a role of the parent coroutine
 * for @a awaiter. When this coroutine resumes it calls try_schedule
 * for its parent coroutine.
 *
 * \since v.5.8.7
 */
template< typename Source_Awaiter >
[[nodiscard]]
resumption_by_specific_scheduler_task_t< Source_Awaiter >
wrap_foreign_awaiter(
	//! Scheduler to be used for resumption.
	so_5::cpp_coro::scheduler_t & scheduler,
	//! Foreign awaiter object. It's expected that this pointer is not null.
	Source_Awaiter * awaiter )
	{
		(void) scheduler;
		(void) awaiter;

		using promise_type =
				typename resumption_by_specific_scheduler_task_t< Source_Awaiter >
						::promise_type;

		auto this_coro_handle =
				co_await so_5::cpp_coro::details::current_coro_handle();

		// We have to get an lvalue instance of coroutine_handle<promise_type>.
		// It allows us to get non-const reference to the promise object.
		std::coroutine_handle< promise_type > this_coro =
				std::coroutine_handle< promise_type >::from_address(
						this_coro_handle.address() );

		// The underlying task has to be scheduled on the proper scheduler.
		this_coro.promise().m_task_data->m_resumable_item.try_schedule();
	}

//
// is_so5_awaiter
//
//! Helper concept for checking Awaiter type.
/*!
 * All SO-5 related awaitables have to have so5_awaitable inner type.
 *
 * \since 5.8.7
 */
template< typename T >
concept is_so5_awaiter = requires {
	typename T::so5_awaitable;
};

} /* namespace task_impl */

//
// task_t
//
/*!
 * @brief A task template that implements awaiter for value of type T.
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
		//! Special marker that this type is a SO-5 awaiter.
		using so5_awaitable = void;

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

				//! Special wrapper for Awater object.
				/*!
				 * If the @a awaiter is instance of SObjectizer-related awaitable
				 * then it does nothing and returns @a awaiter. But if @a awaiter
				 * is a foreign awaitable the it will be wrapped into
				 * resumption_by_specific_scheduler_task_t instance and this new
				 * instance will be returned.
				 *
				 * This transformation allows resumption of SObjectizer-related
				 * coroutines only by appropriate SObjectizer-related schedulers.
				 */
				template< typename Awaiter >
				auto
				await_transform( Awaiter && awaiter )
					{
						using namespace task_impl;

						if constexpr( is_so5_awaiter< Awaiter > )
							{
								return static_cast<Awaiter &&>(awaiter);
							}
						else
							{
								return wrap_foreign_awaiter(
										current_scheduler_setter_t::safe_get(),
										std::addressof(awaiter) );
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


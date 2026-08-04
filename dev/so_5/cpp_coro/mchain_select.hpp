/*
 * SObjectizer-5
 */

/// @file
/// @brief Staff related to coroutine-friendly select function.
///
/// @since v.5.8.7

#if !defined( SO_5_CPP_CORO_MCHAIN_SELECT_HPP )
#define SO_5_CPP_CORO_MCHAIN_SELECT_HPP

#include <so_5/declspec.hpp>

#include <so_5/cpp_coro/details/final_awaiter.hpp>
#include <so_5/cpp_coro/details/current_coro_handle.hpp>

#include <so_5/cpp_coro/scheduler.hpp>

#include <so_5/message.hpp>
#include <so_5/mchain_select.hpp>
#include <so_5/timers.hpp>
#include <so_5/environment.hpp>
#include <so_5/send_functions.hpp>

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

namespace async_select_impl
{

//FIXME: document this!
class async_select_notificator_t final
	: public so_5::mchain_props::select_notificator_t
	{
	private :
		std::mutex m_lock;

		//! Scheduler to be used for resumption.
		scheduler_t & m_scheduler;

		//! Information for resumption.
		resumable_item_t m_to_be_resumed;

		//! Queue of already notified select_cases.
		so_5::mchain_props::select_case_t * m_tail = nullptr;

		/*!
		 * \attention This method must be called only on locked object.
		 */
		void
		push_to_notified_chain(
			so_5::mchain_props::select_case_t & what ) noexcept
			{
				what.set_next( m_tail );
				m_tail = &what;
			}

	public :
		/*!
		 * \brief Initializing constructor.
		 *
		 * Intended to be used with select_cases_holder and its iterators.
		 *
		 * Every select_case is automatically added to the list of notified
		 * select_cases.
		 */
		template< typename Fwd_it >
		async_select_notificator_t(
			//FIXME: document this!
			scheduler_t & scheduler,
			//FIXME: document this!
			std::coroutine_handle<> coro_to_resume,
			Fwd_it b, Fwd_it e )
			: m_scheduler{ scheduler }
			, m_to_be_resumed{ coro_to_resume }
			{
				// All select_cases from range [b,e) must be included in
				// ready_cases list.
				while( b != e )
					{
						b->set_next( m_tail );
						m_tail = &(*b);
						++b;
					}
			}

		void
		notify(
			so_5::mchain_props::select_case_t & what ) noexcept override
			{
				so_5::mchain_props::select_case_t * old_tail = nullptr;
				{
					std::lock_guard< std::mutex > lock{ m_lock };

					old_tail = m_tail;
					push_to_notified_chain( what );
				}

				if( !old_tail )
					m_scheduler.schedule( m_to_be_resumed );
			}

		/*!
		 * \brief Return specifed select_case object to the chain of
		 * 'notified select_cases'.
		 *
		 * If a message has been read from a mchain then there could be
		 * other messages in that mchain. Because of that the select_case
		 * for that mchain must be seen as 'notified' -- it should be
		 * processed on next call to wait() method. This method must be
		 * used for immediately return of select_case to the chain of
		 * 'notified select_cases'.
		 */
		void
		return_to_ready_chain(
			so_5::mchain_props::select_case_t & what ) noexcept
			{
				std::lock_guard< std::mutex > lock{ m_lock };
				push_to_notified_chain( what );
			}

		//FIXME: document this!
		[[nodiscard]]
		so_5::mchain_props::select_case_t *
		wait(
			//FIXME: document that this parameter is ignored and why.
			so_5::mchain_props::duration_t /*wait_time*/ )
			{
				std::lock_guard< std::mutex > lock{ m_lock };

				auto * result = m_tail;
				m_tail = nullptr;

				return result;
			}
	};

//FIXME: document this!
using defined_select_params_t =
		mchain_select_params_t< so_5::mchain_props::msg_count_status_t::defined >;

//FIXME: document this!
class SO_5_TYPE abstract_empty_timeout_handler_t
	{
	protected:
		virtual ~abstract_empty_timeout_handler_t();

	public:
		//FIXME: document this!
		virtual
		void
		schedule() = 0;

		[[nodiscard]] virtual
		bool
		is_elapsed() = 0;

		static
		void
		noop_deleter( abstract_empty_timeout_handler_t * ) noexcept;

		static
		void
		real_deleter( abstract_empty_timeout_handler_t * ) noexcept;
	};

using empty_timeout_handler_unique_ptr_t =
	std::unique_ptr<
			abstract_empty_timeout_handler_t,
			void (*)( abstract_empty_timeout_handler_t * ) >;

//
// empty_timeout_handling_tools_t
//
//FIXME: document this!
struct SO_5_TYPE empty_timeout_handling_tools_t
	{
		struct no_timeout_t {};

		empty_timeout_handler_unique_ptr_t m_handler;
		so_5::mchain_props::select_case_unique_ptr_t m_select_case;

		// For case when empty timeout is not used.
		empty_timeout_handling_tools_t( no_timeout_t );

		// For case when empty timeout is used.
		empty_timeout_handling_tools_t(
			empty_timeout_handler_unique_ptr_t handler,
			so_5::mchain_props::select_case_unique_ptr_t select_case );
	};

//
// no_empty_timeout_handler_t
//
//FIXME: document this!
class SO_5_TYPE no_empty_timeout_handler_t final
	: public abstract_empty_timeout_handler_t
	{
	public:
		no_empty_timeout_handler_t() = default;
		~no_empty_timeout_handler_t() = default;

		void
		schedule() override;

		bool
		is_elapsed() override;
	};

//FIXME: document this!
class SO_5_TYPE actual_no_empty_timeout_handler_t final
	: public abstract_empty_timeout_handler_t
	{
	public:
		/// Message to be sent as delayed.
		struct SO_5_TYPE msg_elapsed final : so_5::message_t
			{
				/// Value to be checked when message arrives.
				unsigned long m_id;

				msg_elapsed( unsigned long id );
			};

	private:
		//FIXME: document this!
		so_5::mchain_t m_target;
	
		/// Value of no_wait_on_empty timeout.
		so_5::mchain_props::duration_t m_timeout;

		/// ID of scheduled delayed message.
		so_5::timer_id_t m_timer_id;

		/// Actual value to be checked when `msg_elapsed` arrives.
		unsigned long m_actual_id;

		/// Is empty timeout elapsed?
		bool m_is_elapsed;

	public:
		//FIXME: document this!
		actual_no_empty_timeout_handler_t(
			so_5::mchain_t target,
			so_5::mchain_props::duration_t timeout );
		~actual_no_empty_timeout_handler_t();

		void
		schedule() override;

		bool
		is_elapsed() override;

		/// @note
		/// This method has to be called when msg_elapsed arrives
		/// to the target mchain.
		void
		react_to( const msg_elapsed & cmd );
	};

//FIXME: document this!
inline no_empty_timeout_handler_t no_empty_timeout_handler;

//FIXME: maybe it's better to place it into .cpp-file?
[[nodiscard]] inline
empty_timeout_handling_tools_t
make_empty_timeout_handling_tools(
	so_5::environment_t & env,
	const defined_select_params_t & params )
	{
		using so_5::mchain_props::details::infinite_wait_special_timevalue;

std::cout << "make_empty_timeout_handling_tools 0" << std::endl;
		if( infinite_wait_special_timevalue() == params.empty_timeout() )
			{
std::cout << "make_empty_timeout_handling_tools 1" << std::endl;
				return { empty_timeout_handling_tools_t::no_timeout_t{} };
			}
std::cout << "empty_timeout=" << params.empty_timeout() << std::endl;
std::cout << "make_empty_timeout_handling_tools 2" << std::endl;
		// We need a mchain for delayed msg_elapsed message.
		auto ch = env.create_mchain(
				so_5::make_limited_without_waiting_mchain_params(
						// just one message to be received
						1u,
						// Storage has to be preallocated.
						so_5::mchain_props::memory_usage_t::preallocated,
						// Just throw out the oldest message.
						so_5::mchain_props::overflow_reaction_t::remove_oldest ) );

		// Now we can make actual empty timeout handler.
		//
		// NOTE: handler_raw_ptr is actual_no_empty_timeout_handler_t.
		// Then it will be wrapped into empty_timeout_handler_unique_ptr_t
		// and lost its precise type. But we need a pointer of the type
		// actual_no_empty_timeout_handler_t.
		//
		// ATTENTION: we expect that there won't be throwing actions
		// between this new and declaration of `handler` wariable.
		auto * handler_raw_ptr = new actual_no_empty_timeout_handler_t{
						ch,
						params.empty_timeout()
					};
		empty_timeout_handler_unique_ptr_t handler{
				handler_raw_ptr,
				&abstract_empty_timeout_handler_t::real_deleter
			};

		using msg_elapsed = actual_no_empty_timeout_handler_t::msg_elapsed;
		auto select_case = so_5::receive_case(
				ch,
				[handler_raw_ptr]( const msg_elapsed & cmd ) {
					handler_raw_ptr->react_to( cmd );
				} );

		return { std::move(handler), std::move(select_case) };
	}

//FIXME: document this!
/// @attention
/// @a empty_timeout_handler should not be nullptr.
template<
	std::size_t Cases_Count >
resumable_select_t
do_select_without_total_time(
	scheduler_t & coro_scheduler,
	defined_select_params_t params,
	so_5::mchain_props::details::select_cases_holder_t< Cases_Count > cases_holder )
	{
		using namespace so_5::mchain_props::details;

		using holder_t = so_5::mchain_props::details
				::select_cases_holder_t< Cases_Count >;

		using performer_t = select_actions_performer_t<
				holder_t,
				async_select_notificator_t >;

std::cout << "here (0)!" << std::endl;

		auto empty_timeout_tools = make_empty_timeout_handling_tools(
				coro_scheduler.environment(),
				params );

std::cout << "here (1)!" << std::endl;

		async_select_notificator_t notificator{
				coro_scheduler,
				co_await so_5::cpp_coro::details::current_coro_handle(),
				cases_holder.begin(),
				cases_holder.end()
			};

		if( empty_timeout_tools.m_select_case )
			{
				// The select case for empty timeout has to be added to
				// ready chain of the notificator.
				notificator.return_to_ready_chain(
						*(empty_timeout_tools.m_select_case) );
			}

		performer_t performer{
				params,
				cases_holder,
				notificator
			};

		//FIXME: can this loop be moved into a separate function?
		do
			{
				// Empty timeout has to be scheduled before entering
				// into handle_next.
				empty_timeout_tools.m_handler->schedule();

				const auto handle_result = performer.handle_next(
						std::chrono::seconds::zero() );
				if( so_5::mchain_props::extraction_status_t::msg_extracted ==
						performer.last_extraction_status() )
					{
						// Becase some message extracted we must restart empty
						// timeout timer.
						empty_timeout_tools.m_handler->schedule();
					}
				else
					{
						//FIXME: document this!
						if( handle_next_result_t::no_ready_cases == handle_result )
							co_yield resumable_select_t::waiting_for_next_event_t{};

						// There could be one of two situations:
						// 1) several threads do select on the same mchain.
						//    Both threads will be awoken when some message is
						//    pushed into the mchain. But only one thread will get
						//    this message. Second thread will receive no_messages
						//    status.
						//    but wait_time must be decremented.
						// 2) some chain is closed.
					}
			}
		while( performer.can_continue() &&
				!empty_timeout_tools.m_handler->is_elapsed() );

		co_return performer.make_result();
	}

template<
	std::size_t Cases_Count >
resumable_select_t
perform_select(
	/// Scheduler to be used.
	scheduler_t & coro_scheduler,
	defined_select_params_t params,
	so_5::mchain_props::details::select_cases_holder_t< Cases_Count > cases_holder )
	{
//FIXME: has to be implemented!
#if 0
		if( is_infinite_wait_timevalue( params.total_time() ) )
#endif
			return do_select_without_total_time(
					coro_scheduler,
					std::move(params),
					std::move(cases_holder) );
//FIXME: has to be implemented!
#if 0
		else
			return do_select_with_total_time( params, cases_holder );
#endif
	}

} /* namespace async_select_impl */

//FIXME: document this!
template<
	so_5::mchain_props::msg_count_status_t Msg_Count_Status,
	typename... Cases >
resumable_select_t
select(
	//FIXME: document this!
	scheduler_t & coro_scheduler,
	//! Parameters for advanced select.
	so_5::mchain_select_params_t< Msg_Count_Status > params,
	//! Select cases.
	Cases &&... cases )
	{
		static_assert(
				Msg_Count_Status == so_5::mchain_props::msg_count_status_t::defined,
				"message count to be processed/extracted should be defined "
				"by using handle_all()/handle_n()/extract_n() methods" );

		using namespace so_5::mchain_props;
		using namespace so_5::mchain_props::details;

		select_cases_holder_t< sizeof...(cases) > cases_holder;
		fill_select_cases_holder(
				cases_holder, 0, std::forward< Cases >(cases)... );

		return async_select_impl::perform_select(
				coro_scheduler,
				std::move(params),
				std::move(cases_holder) );
	}

} /* namespace so_5::cpp_coro */

#endif


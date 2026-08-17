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

std::cout << "here (1)!" << std::endl;

		async_select_notificator_t notificator{
				coro_scheduler,
				co_await so_5::cpp_coro::details::current_coro_handle(),
				cases_holder.begin(),
				cases_holder.end()
			};

		performer_t performer{
				params,
				cases_holder,
				notificator
			};

		//FIXME: can this loop be moved into a separate function?
		do
			{
				const auto handle_result = performer.handle_next(
						std::chrono::seconds::zero() );
				if( so_5::mchain_props::extraction_status_t::msg_extracted ==
						performer.last_extraction_status() )
					{
//FIXME: empty timeout has to be rescheduled.
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
		while( performer.can_continue() );

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


#include <so_5/all.hpp>

#include <so_5/cpp_coro/scheduler.hpp>
#include <so_5/cpp_coro/mchain_select.hpp>

#include <so_5/cpp_coro/this_thread_scheduler.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

namespace test
{

using so_5::cpp_coro::this_thread_scheduler_impl::top_level_task_t;

class this_agent_scheduler_t final
	: public so_5::cpp_coro::scheduler_t
{
public:
	struct msg_resume_this final : public so_5::message_t
	{
		struct ordinary {};
		struct delayed {};

		so_5::cpp_coro::resumable_item_t * m_what;

		msg_resume_this(
			so_5::cpp_coro::resumable_item_t & what,
			ordinary )
			: m_what{ std::addressof(what) }
		{
			std::cout << this << " msg_resume_this(ordinary) created, what="
					<< m_what << std::endl;
		}

		msg_resume_this(
			so_5::cpp_coro::resumable_item_t & what,
			delayed )
			: m_what{ std::addressof(what) }
		{
			std::cout << this << " msg_resume_this(delayed) created, what="
					<< m_what << std::endl;
		}

		~msg_resume_this()
		{
			std::cout << this << " msg_resume_this deleted, what="
					<< m_what << std::endl;
		}
	};

private:
	struct notification_messages_t
	{
		so_5::message_ref_t m_ordinary_resume;

		so_5::timer_id_t m_timer;
		so_5::message_ref_t m_timeout_elapsed;

		~notification_messages_t()
		{
			cancel_ordinary_resume();
			cancel_timeout_elapsed();

			std::cout << "notification_messages_t destroyed" << std::endl;
		}

		void
		cancel_all()
		{
			cancel_ordinary_resume();
			cancel_timeout_elapsed();
		}

		void
		cancel_ordinary_resume()
		{
			if( m_ordinary_resume )
			{
				auto & msg = dynamic_cast<msg_resume_this &>(*m_ordinary_resume);
				msg.m_what = nullptr;

				std::cout << "ordinary msg_resume_this cancelled, msg="
						<< std::addressof(msg) << std::endl;
			}

			m_ordinary_resume.reset();
		}

		void
		cancel_timeout_elapsed()
		{
			if( m_timeout_elapsed )
			{
				m_timer.release();

				auto & msg = dynamic_cast<msg_resume_this &>(*m_timeout_elapsed);
				msg.m_what = nullptr;

				std::cout << "delayed msg_resume_this cancelled, msg="
						<< std::addressof(msg) << std::endl;
			}

			m_timeout_elapsed.reset();
		}

		static void
		deleter( void * info ) noexcept
		{
			delete static_cast<notification_messages_t *>(info);
		}

		[[nodiscard]] static
		so_5::cpp_coro::resumable_item_t::scheduler_data_t::extra_data_t
		factory()
		{
			auto info = std::make_unique< notification_messages_t >();
			return { info.release(), &notification_messages_t::deleter };
		}
	};

	const so_5::mbox_t m_dest;

	std::mutex m_lock;

public:
	this_agent_scheduler_t( so_5::mbox_t dest )
		: m_dest{ std::move(dest) }
	{}

	so_5::environment_t &
	environment() const override
	{
		return m_dest->environment();
	}

	void
	try_schedule(
		so_5::cpp_coro::resumable_item_t & what_to_resume ) override
	{
		std::lock_guard lock{ m_lock };

		switch( what_to_resume.scheduler_data().m_status )
		{
		case so_5::cpp_coro::resumable_item_t::status_t::neutral:
		case so_5::cpp_coro::resumable_item_t::status_t::notified:
			// There is no need to schedule the coroutine because it's
			// running now.
		break;

		case so_5::cpp_coro::resumable_item_t::status_t::ready:
			// There is no need to schedule the coroutine because it's
			// already scheduled.
		break;

		case so_5::cpp_coro::resumable_item_t::status_t::waiting:
			what_to_resume.scheduler_data().m_status =
					so_5::cpp_coro::resumable_item_t::status_t::ready;

			auto & extra_data = what_to_resume.scheduler_data()
					.m_extra_data.get_or_create< notification_messages_t >(
							&notification_messages_t::factory );

			// NOTE: it's important that extra_data.cancel_timeout_elapsed
			// is not called here.

			extra_data.m_ordinary_resume =
					std::make_unique< msg_resume_this >(
							what_to_resume,
							msg_resume_this::ordinary{} );
			so_5::send(
					m_dest,
					so_5::mhood_t< msg_resume_this >(
							extra_data.m_ordinary_resume ) );
		break;
		}
	}

	so_5::cpp_coro::try_suspend_result_t
	try_suspend(
		so_5::cpp_coro::resumable_item_t & what_to_handle,
		so_5::cpp_coro::monotonic_clock_t::duration sleep_time ) override
	{
		std::lock_guard lock{ m_lock };

		auto & scheduler_data = what_to_handle.scheduler_data();

		switch( scheduler_data.m_status )
		{
		case so_5::cpp_coro::resumable_item_t::status_t::neutral:
			// This case will be handled outside of the switch.
		break;

		case so_5::cpp_coro::resumable_item_t::status_t::notified:
			// Coroutine is running now. The status has to be changed, but
			// coroutine shouldn't be suspended.
			scheduler_data.m_status =
					so_5::cpp_coro::resumable_item_t::status_t::neutral;

			return so_5::cpp_coro::try_suspend_result_t::should_be_resumed;

		case so_5::cpp_coro::resumable_item_t::status_t::ready:
			// This shouldn't happen.
			throw std::runtime_error{
					"a ready to run coroutine can't be suspended"
				};
		break;

		case so_5::cpp_coro::resumable_item_t::status_t::waiting:
			// This shouldn't happen.
			throw std::runtime_error{
					"try_suspend is called for already suspended coroutine"
				};
		break;
		}

		scheduler_data.m_status =
				so_5::cpp_coro::resumable_item_t::status_t::waiting;

//FIXME: it seems that this code is redundant.
//the extra_data has actual timeout_elapsed message only when
//resumable_item status is `waiting`, but this status has been set
//just right now.
#if 0
		if( scheduler_data.m_extra_data.has_value() )
			scheduler_data.m_extra_data.as_ref< notification_messages_t >()
					.cancel_delivery();
#endif

		if( so_5::cpp_coro::infinite_speep_time() != sleep_time )
		{
			auto & notifications = scheduler_data.m_extra_data
					.get_or_create< notification_messages_t >(
							notification_messages_t::factory );

			notifications.m_timeout_elapsed =
					std::make_unique< msg_resume_this >(
							what_to_handle,
							msg_resume_this::delayed{} );

			notifications.m_timer = so_5::send_periodic(
					m_dest,
					sleep_time,
					std::chrono::milliseconds::zero(),
					so_5::mhood_t< msg_resume_this >(
							notifications.m_timeout_elapsed ) );
		}

		std::cout << "coro will be suspended "
			<< what_to_handle.scheduler_data().m_to_be_resumed.address()
			<< " (" << std::addressof(what_to_handle) << ")"
			<< std::endl;

		return so_5::cpp_coro::try_suspend_result_t::should_be_suspended;
	}

	void
	do_resumption( const msg_resume_this & cmd )
	{
		if( cmd.m_what )
		{
			auto & scheduler_data = cmd.m_what->scheduler_data();

			// Status for this coroutine has to be changed.
			{
				std::lock_guard lock{ m_lock };
				scheduler_data.m_status =
						so_5::cpp_coro::resumable_item_t::status_t::neutral;
			}

			// If there are pending notifications they could be cancelled.
			if( auto * notifications = scheduler_data.m_extra_data
					.as_ptr< notification_messages_t >() )
			{
				notifications->cancel_all();
			}

			std::cout << "coro to be resumed via msg_resume_this: "
					<< scheduler_data.m_to_be_resumed.address()
					<< " (" << std::addressof(cmd) << ")"
					<< std::endl;

			scheduler_data.m_to_be_resumed.resume();
		}
	}
};

class a_demo_t final : public so_5::agent_t
{
	struct msg_catch_this final : public so_5::signal_t {};

	struct msg_initiate_receive_message_async final : public so_5::signal_t {};

	this_agent_scheduler_t m_scheduler;

	std::optional< so_5::cpp_coro::task_t< void > > m_current_top_level_coro;

	const so_5::mchain_t m_ch;

	so_5::cpp_coro::task_t< void >
	receive_message_async()
	{
		std::cout << "receive_message_async started" << std::endl;

		co_await so_5::cpp_coro::select(
				m_scheduler,
				so_5::from_all()
						.handle_n(3)
						//.empty_timeout( std::chrono::seconds{1} )
						,
				so_5::receive_case( m_ch,
					[]( so_5::mhood_t<msg_catch_this> )
					{
						std::cout << "signal received" << std::endl;
					} )
			);

		std::cout << "receive_message_async finished" << std::endl;

		so_deregister_agent_coop_normally();
	}

	void
	check_and_handle_top_level_coro_completion()
	{
		if( m_current_top_level_coro.has_value()
				&& m_current_top_level_coro->m_coro.done() )
		{
			m_current_top_level_coro.reset();
		}
	}

public:
	a_demo_t( context_t ctx )
		: so_5::agent_t{ std::move(ctx) }
		, m_scheduler{ so_direct_mbox() }
		, m_ch{ so_5::create_mchain( so_environment() ) }
	{}

	void
	so_define_agent() override
	{
		so_subscribe_self()
			.event( &a_demo_t::evt_initiate_receive_message_async )
			.event( &a_demo_t::evt_resume_coro )
			;
	}

	void
	so_evt_start() override
	{
		so_5::send< msg_initiate_receive_message_async >( *this );

		so_5::send_delayed< msg_catch_this >( m_ch,
				std::chrono::milliseconds{ 50 } );
		so_5::send_delayed< msg_catch_this >( m_ch,
				std::chrono::milliseconds{ 80 } );
		so_5::send_delayed< msg_catch_this >( m_ch,
				std::chrono::milliseconds{ 110 } );
	}

	void
	so_evt_finish() override
	{
		std::cout << "work finished" << std::endl;
	}

private:
	void
	evt_initiate_receive_message_async(
		mhood_t< msg_initiate_receive_message_async > )
	{
		auto wrapped_coro = [this]() -> top_level_task_t< void >
		{
			co_return co_await receive_message_async();
		};
		m_current_top_level_coro = wrapped_coro();
		std::cout << "coro to be resumed: "
				<< m_current_top_level_coro->m_coro.address()
				<< std::endl;
		m_current_top_level_coro->m_coro.resume();
		check_and_handle_top_level_coro_completion();
	}

	void
	evt_resume_coro(
		mhood_t< this_agent_scheduler_t::msg_resume_this > cmd )
	{
		std::cout << "evt_resume_coro " << cmd.get() << std::endl;
		m_scheduler.do_resumption( *cmd );
		std::cout << "do_resumption completed" << std::endl;
		check_and_handle_top_level_coro_completion();
	}
};

void
run_demo()
{
	so_5::launch( []( so_5::environment_t & env ) {
			env.register_agent_as_coop(
					env.make_agent< a_demo_t >() );
		} );
}

} /* namespace test */

using namespace test;

int
main()
{
	try
	{
		run_with_time_limit(
			[]() {
				run_demo();
			},
			5 );
	}
	catch(const std::exception & ex)
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


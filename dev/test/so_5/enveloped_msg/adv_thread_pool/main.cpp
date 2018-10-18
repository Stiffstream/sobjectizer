/*
 * Check for enveloped message and adv_thread_pool dispatcher.
 */

#include <so_5/all.hpp>
#include <so_5/h/stdcpp.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

#include <utest_helper_1/h/helper.hpp>

#include "../common_stuff.hpp"

struct thread_safe_action final : public so_5::signal_t {};

struct thread_unsafe_action final : public so_5::signal_t {};

struct shutdown final : public so_5::signal_t {};

class test_case_t final : public so_5::agent_t
{
	trace_t m_trace;
	const so_5::mbox_t m_mbox;

	std::mutex m_lock;
	std::set< so_5::current_thread_id_t > m_active_threads;
	int m_active_handlers{};

public :
	test_case_t( context_t ctx )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_mbox{ special_mbox_t<>::make(
				so_environment().create_mbox(),
				so_5::outliving_mutable(m_trace),
				"mbox" ) }
	{
		so_subscribe( m_mbox )
				.event( &test_case_t::on_safe_action, so_5::thread_safe )
				.event( &test_case_t::on_unsafe_action );

		so_subscribe_self()
				.event( [this](mhood_t<shutdown>) {
							so_deregister_agent_coop_normally();
						} );
	}

	void
	so_evt_start() override
	{
		so_5::send< thread_safe_action >( m_mbox );
		so_5::send< thread_safe_action >( m_mbox );
		so_5::send< thread_safe_action >( m_mbox );

		so_5::send< thread_unsafe_action >( m_mbox );
		so_5::send< thread_unsafe_action >( m_mbox );
		so_5::send< thread_unsafe_action >( m_mbox );

		so_5::send< shutdown >( *this );
	}

	void
	so_evt_finish() override
	{
		ensure_or_die( 3 == m_active_threads.size(),
				"m_active_threads.size() is expected to be 3" );

		std::cout << "Trace is: " << m_trace.content() << std::endl;
		ensure_or_die( !m_trace.content().empty(),
				"trace should not be empty!" );
	}

private :
	void
	on_safe_action( mhood_t<thread_safe_action> )
	{
		{
			std::lock_guard< std::mutex > lock{ m_lock };
			m_active_threads.insert( so_5::query_current_thread_id() );
			++m_active_handlers;
		}

		std::this_thread::sleep_for( std::chrono::milliseconds(250) );

		{
			std::lock_guard< std::mutex > lock{ m_lock };
			--m_active_handlers;
		}
	}

	void
	on_unsafe_action( mhood_t<thread_unsafe_action> )
	{
		{
			std::lock_guard< std::mutex > lock{ m_lock };
			ensure_or_die( 0 == m_active_handlers, "m_active_handlers must be 0" );
			++m_active_handlers;
		}

		std::this_thread::sleep_for( std::chrono::milliseconds(100) );

		{
			std::lock_guard< std::mutex > lock{ m_lock };
			ensure_or_die( 1 == m_active_handlers, "m_active_handlers must be 1" );
			--m_active_handlers;
		}
	}
};

void
run_test()
{
	so_5::launch( [&]( so_5::environment_t & env ) {
			env.introduce_coop( [&]( so_5::coop_t & coop ) {
					coop.make_agent_with_binder< test_case_t >(
							so_5::disp::adv_thread_pool::create_private_disp(
									env, 6 )->binder(
											so_5::disp::adv_thread_pool::bind_params_t{} ) );
				} );
		},
		[]( so_5::environment_params_t & params ) {
			params.message_delivery_tracer(
					so_5::msg_tracing::std_cout_tracer() );
		} );
}

int
main()
{
	try
	{
		run_with_time_limit(
			[]() {
				run_test();
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


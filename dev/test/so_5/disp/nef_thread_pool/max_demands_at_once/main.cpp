/*
 * A test for nef_thread_pool dispatcher that checks max_demands_at_once
 * parameter.
 */

#include <iostream>
#include <set>
#include <exception>
#include <stdexcept>
#include <cstdlib>
#include <thread>
#include <chrono>

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

#include "../for_each_lock_factory.hpp"

namespace test
{

struct msg_notice final : public so_5::message_t
{
	std::string m_info;

	msg_notice( std::string info )
		:	m_info{ std::move(info) }
	{}
};

struct msg_completed final : public so_5::signal_t {};

class a_supervisor_t final : public so_5::agent_t
{
public:
	a_supervisor_t( context_t ctx, std::size_t agents_count )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_agents_count{ agents_count }
	{}

	void
	so_define_agent() override
	{
		so_subscribe_self()
			.event( &a_supervisor_t::evt_info )
			.event( &a_supervisor_t::evt_completed )
			;
	}

private:
	const std::size_t m_agents_count;

	std::string m_log;

	std::size_t m_msg_completed_received{};

	void
	evt_info( mhood_t<msg_notice> cmd )
	{
		m_log += cmd->m_info;
		m_log += ";";
	}

	void
	evt_completed( mhood_t<msg_completed> )
	{
		++m_msg_completed_received;
		if( m_msg_completed_received == m_agents_count )
		{
			const std::string expected_log =
				"a-1;a-2;a-3;b-1;b-2;b-3;c-1;c-2;c-3;"
				"a-4;a-5;b-4;b-5;c-4;c-5;";

			ensure_or_die( expected_log == m_log,
					"unexpected log: '"
					+ m_log
					+ "'; expected log: '"
					+ expected_log
					+ "'" );

			so_deregister_agent_coop_normally();
		}
	}
};

class a_test_t : public so_5::agent_t
{
		struct msg_1 final : public so_5::signal_t {};
		struct msg_2 final : public so_5::signal_t {};
		struct msg_3 final : public so_5::signal_t {};
		struct msg_4 final : public so_5::signal_t {};
		struct msg_5 final : public so_5::signal_t {};

		const std::string m_name;
		const so_5::mbox_t m_target_mbox;

	public:
		a_test_t(
			context_t ctx,
			so_5::priority_t priority,
			std::string name,
			so_5::mbox_t target_mbox )
			:	so_5::agent_t( ctx + priority )
			,	m_name{ std::move(name) }
			,	m_target_mbox{ std::move(target_mbox) }
		{}

		void
		so_define_agent() override
		{
			so_subscribe_self()
				.event( [this](mhood_t<msg_1>) { handle_evt( "1" ); } )
				.event( [this](mhood_t<msg_2>) { handle_evt( "2" ); } )
				.event( [this](mhood_t<msg_3>) { handle_evt( "3" ); } )
				.event( [this](mhood_t<msg_4>) { handle_evt( "4" ); } )
				.event( [this](mhood_t<msg_5>) {
						handle_evt( "5" );
						so_5::send< msg_completed >( m_target_mbox );
					} )
				;
		}

		void
		so_evt_start() override
		{
			so_5::send< msg_1 >( *this );
			so_5::send< msg_2 >( *this );
			so_5::send< msg_3 >( *this );
			so_5::send< msg_4 >( *this );
			so_5::send< msg_5 >( *this );
		}

	private:
		void
		handle_evt( const std::string & msg_name )
		{
			so_5::send< msg_notice >( m_target_mbox, m_name + "-" + msg_name );
		}
};

void
do_test()
{
	using namespace so_5::disp::nef_thread_pool;
	for_each_lock_factory( []( queue_traits::lock_factory_t factory ) {
		run_with_time_limit( [&]()
			{
				so_5::launch(
					[&]( so_5::environment_t & env )
					{
						env.introduce_coop( [&factory]( so_5::coop_t & coop )
							{
								auto disp = make_dispatcher( coop.environment(),
										std::string_view{},
										disp_params_t{}
											.thread_count( 1u )
											.set_queue_params(
												queue_traits::queue_params_t{}
													.lock_factory( factory ) ) );

								// Will work on the default dispatcher.
								auto * supervisor = coop.make_agent< a_supervisor_t >( 3u );

								bind_params_t bind_params;
								bind_params.max_demands_at_once( 4u );

								// Use the fact that agents are sorted inside
								// the coop with respect to the priority.
								// If all agents have the same priority then
								// agent 'c' can receive evt_start before
								// agent 'a', and agent 'b' can receive evt_start
								// before agent 'c'.
								coop.make_agent_with_binder< a_test_t >(
										disp.binder( bind_params ),
										so_5::prio::p3,
										"a",
										supervisor->so_direct_mbox() );
								coop.make_agent_with_binder< a_test_t >(
										disp.binder( bind_params ),
										so_5::prio::p2,
										"b",
										supervisor->so_direct_mbox() );
								coop.make_agent_with_binder< a_test_t >(
										disp.binder( bind_params ),
										so_5::prio::p1,
										"c",
										supervisor->so_direct_mbox() );
							} );
					} );
			},
			20 );
	} );
}

} /* namespace test */

using namespace test;

int
main()
{
	try
	{
		do_test();
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


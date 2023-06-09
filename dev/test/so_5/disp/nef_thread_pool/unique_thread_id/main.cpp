/*
 * A test for nef_thread_pool dispatcher that checks that all threads
 * in pool are used.
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

struct msg_thread_info final : public so_5::message_t
{
	so_5::current_thread_id_t m_thread_id;

	msg_thread_info( so_5::current_thread_id_t thread_id )
		:	m_thread_id{ thread_id }
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
			.event( &a_supervisor_t::evt_thread_info )
			.event( &a_supervisor_t::evt_completed )
			;
	}

private:
	const std::size_t m_agents_count;

	std::set< so_5::current_thread_id_t > m_threads;

	std::size_t m_msg_completed_received{};

	void
	evt_thread_info( mhood_t<msg_thread_info> cmd )
	{
		m_threads.insert( cmd->m_thread_id );
	}

	void
	evt_completed( mhood_t<msg_completed> )
	{
		++m_msg_completed_received;
		if( m_msg_completed_received == m_agents_count )
		{
			ensure_or_die( m_agents_count == m_threads.size(),
					"unexpected number of unique thread IDs: "
					+ std::to_string( m_threads.size() )
					+ "; expected value: "
					+ std::to_string( m_agents_count ) );

			so_deregister_agent_coop_normally();
		}
	}
};

class a_test_t : public so_5::agent_t
{
		const so_5::mbox_t m_target_mbox;

	public:
		a_test_t( context_t ctx, so_5::mbox_t target_mbox )
			:	so_5::agent_t( std::move(ctx) )
			,	m_target_mbox{ std::move(target_mbox) }
		{}

		void
		so_evt_start() override
		{
			so_5::send< msg_thread_info >( m_target_mbox,
					so_5::query_current_thread_id() );

			std::this_thread::sleep_for( std::chrono::milliseconds{ 250 } );

			so_5::send< msg_completed >( m_target_mbox );
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
								constexpr std::size_t agents_count = 4u;
								auto disp = make_dispatcher( coop.environment(),
										std::string_view{},
										disp_params_t{}
											.thread_count( agents_count )
											.set_queue_params(
												queue_traits::queue_params_t{}
													.lock_factory( factory ) ) );

								// Will work on the default dispatcher.
								auto * supervisor = coop.make_agent< a_supervisor_t >(
										agents_count );

								for( std::size_t i = 0; i < agents_count; ++i )
									coop.make_agent_with_binder< a_test_t >(
											disp.binder( bind_params_t{} ),
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


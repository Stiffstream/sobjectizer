/*
 * A test for calling environment_t::stop() for stopping SObjectizer.
 */

#include <iostream>
#include <map>
#include <exception>
#include <stdexcept>
#include <cstdlib>
#include <thread>
#include <chrono>

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

struct msg_hello : public so_5::signal_t {};

template< typename Dispatcher_Handle >
void
make_coop( so_5::environment_t & env, Dispatcher_Handle disp )
{
	using handler_t = std::function< void() >;

	class actor_t final : public so_5::agent_t
	{
		handler_t m_on_start;

	public:
		actor_t( context_t ctx )
			:	so_5::agent_t{ std::move(ctx) }
			,	m_on_start{ [](){} }
		{}

		void on_start( handler_t lambda ) { m_on_start = std::move(lambda); }

		void event( handler_t lambda )
		{
			so_subscribe_self().event(
				[lambda = std::move(lambda)]( mhood_t<msg_hello> ) {
					lambda();
				} );
		}

		void so_evt_start() override
		{
			m_on_start();
		}
	};

	env.introduce_coop( disp.binder(),
		[&]( so_5::coop_t & coop ) {
			auto a1 = coop.make_agent<actor_t>();
			auto a2 = coop.make_agent<actor_t>();
			auto a3 = coop.make_agent<actor_t>();
			auto a4 = coop.make_agent<actor_t>();
			auto a5 = coop.make_agent<actor_t>();
			auto a6 = coop.make_agent<actor_t>();
			auto a7 = coop.make_agent<actor_t>();
			auto a8 = coop.make_agent<actor_t>();

			using namespace so_5;

			a1->on_start( [a2] { send< msg_hello >( *a2 ); } );

			a1->event( [a2] { send< msg_hello >( *a2 ); } );
			a2->event( [a3] { send< msg_hello >( *a3 ); } );
			a3->event( [a4] { send< msg_hello >( *a4 ); } );
			a4->event( [a5] { send< msg_hello >( *a5 ); } );
			a5->event( [a6] { send< msg_hello >( *a6 ); } );
			a6->event( [a7] { send< msg_hello >( *a7 ); } );
			a7->event( [a8] { send< msg_hello >( *a8 ); } );
			a8->event( [a1] { send< msg_hello >( *a1 ); } );
		} );
}

void
make_stopper( so_5::environment_t & env )
{
	class actor_t final : public so_5::agent_t
	{
		struct msg_stop final : public so_5::signal_t {};
	public:
		actor_t( context_t ctx ) : so_5::agent_t{ std::move(ctx) }
		{
			so_subscribe_self().event( [this](mhood_t<msg_stop>) {
					so_environment().stop();
				} );
		}

		void so_evt_start() override
		{
			so_5::send< msg_stop >( *this );
		}
	};

	env.introduce_coop( []( so_5::coop_t & coop ) {
			coop.make_agent< actor_t >();
		} );
}

void
init( so_5::environment_t & env )
{
	auto one_thread = so_5::disp::one_thread::make_dispatcher( env );
	make_coop( env, one_thread );
	make_stopper( env );
}

int
main()
{
	try
	{
		for( int i = 0; i != 1000; ++i )
			run_with_time_limit(
				[]()
				{
					so_5::launch( &init );
				},
				20,
				"stopping environment via environment_t::stop()" );

		std::cout << "done" << std::endl;
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


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

#include <various_helpers_1/time_limited_execution.hpp>

struct msg_hello : public so_5::signal_t {};

template< typename DISPATCHER_HANDLE >
void
make_coop( so_5::environment_t & env, DISPATCHER_HANDLE disp )
{
	env.introduce_coop( disp->binder(),
		[]( so_5::coop_t & coop ) {
			auto a1 = coop.define_agent();
			auto a2 = coop.define_agent();
			auto a3 = coop.define_agent();
			auto a4 = coop.define_agent();
			auto a5 = coop.define_agent();
			auto a6 = coop.define_agent();
			auto a7 = coop.define_agent();
			auto a8 = coop.define_agent();

			using namespace so_5;

			a1.on_start( [a2] { send_to_agent< msg_hello >( a2 ); } );

			a1.event< msg_hello >( a1, [a2] { send_to_agent< msg_hello >( a2 ); } );
			a2.event< msg_hello >( a2, [a3] { send_to_agent< msg_hello >( a3 ); } );
			a3.event< msg_hello >( a3, [a4] { send_to_agent< msg_hello >( a4 ); } );
			a4.event< msg_hello >( a4, [a5] { send_to_agent< msg_hello >( a5 ); } );
			a5.event< msg_hello >( a5, [a6] { send_to_agent< msg_hello >( a6 ); } );
			a6.event< msg_hello >( a6, [a7] { send_to_agent< msg_hello >( a7 ); } );
			a7.event< msg_hello >( a7, [a8] { send_to_agent< msg_hello >( a8 ); } );
			a8.event< msg_hello >( a8, [a1] { send_to_agent< msg_hello >( a1 ); } );
		} );
}

void
make_stopper( so_5::environment_t & env )
{
	env.introduce_coop( []( so_5::coop_t & coop ) {
			struct msg_stop : public so_5::signal_t {};

			auto a1 = coop.define_agent();
			a1.on_start( [a1] { so_5::send_to_agent< msg_stop >( a1 ); } )
				.event< msg_stop >( a1, [&coop] { coop.environment().stop(); } );

		} );
}

void
init( so_5::environment_t & env )
{
	auto one_thread = so_5::disp::one_thread::create_private_disp( env );
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


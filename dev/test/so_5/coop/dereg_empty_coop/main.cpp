/*
 * A unit-test for testing deregistration of empty coop.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

void
init( so_5::rt::environment_t & env )
{
	// Supervison coop.
	env.introduce_coop( []( so_5::rt::agent_coop_t & parent ) {
			auto a = parent.define_agent();
			a.on_start( [&parent, a] {
					// Child coop.
					auto child = so_5::rt::create_child_coop( parent, "child" );
					child->add_reg_notificator(
							so_5::rt::make_coop_reg_notificator( a.direct_mbox() ) );
					child->add_dereg_notificator(
							so_5::rt::make_coop_dereg_notificator( a.direct_mbox() ) );
					parent.environment().register_coop( std::move(child) );
				} )
			.event( a, [&parent]( const so_5::rt::msg_coop_registered & ) {
					parent.environment().deregister_coop(
							"child",
							so_5::rt::dereg_reason::normal );
				} )
			.event( a, [&parent]( const so_5::rt::msg_coop_deregistered & ) {
					parent.deregister_normally();
				} );
		} );
}

int
main()
{
	try
	{
		run_with_time_limit( []{ so_5::launch( init ); },
				5,
				"dereg_empty_coop" );

		return 0;
	}
	catch( const std::exception & x )
	{
		std::cerr << "Exception: " << x.what() << std::endl;
	}

	return 2;
}


/*
 * A unit-test for testing new create_child_coop and introduce_child_coop
 * functions added in v.5.5.8.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

void
init( so_5::environment_t & env )
{
	struct msg_started : public so_5::signal_t {};

	// First cooperation.
	env.introduce_coop( []( so_5::coop_t & parent ) {
			auto a = parent.define_agent();
			a.on_start( [&parent, a] {
					auto child = so_5::create_child_coop( parent, so_5::autoname );
					child->define_agent().on_start( [a] {
						so_5::send< msg_started >( a.direct_mbox() );
					} );
					parent.environment().register_coop( std::move(child) );
				} )
			.event< msg_started >( a, [&parent] { parent.deregister_normally(); } );
		} );

	// Second cooperation.
	env.introduce_coop( []( so_5::coop_t & parent ) {
			auto a = parent.define_agent();
			a.on_start( [&parent, a] {
					so_5::introduce_child_coop( parent,
						[a]( so_5::coop_t & child ) {
							child.define_agent().on_start( [a] {
								so_5::send< msg_started >( a.direct_mbox() );
							} );
						} );
				} )
			.event< msg_started >( a, [&parent] { parent.deregister_normally(); } );
		} );
}

int
main()
{
	try
	{
		so_5::launch( init );

		return 0;
	}
	catch( const std::exception & x )
	{
		std::cerr << "Exception: " << x.what() << std::endl;
	}

	return 2;
}


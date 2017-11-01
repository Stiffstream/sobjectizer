/*
 * A simple test for so_has_subscription method.
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
#include <various_helpers_1/ensure.hpp>

class a_test_t : public so_5::agent_t
{
	struct msg_1 : public so_5::signal_t {};

	state_t st_dummy{ this };
	state_t st_dummy_2{ this };

public :
	a_test_t( context_t ctx )
		:	so_5::agent_t( ctx )
	{}

	virtual void
	so_define_agent() override
	{
		ensure( !so_has_subscription<msg_1>(so_direct_mbox(), so_default_state()),
				"subscription must be absent" );
		ensure( !so_has_subscription<msg_1>(so_direct_mbox()),
				"subscription must be absent" );
		ensure( !so_has_subscription<msg_1>(so_direct_mbox(), st_dummy),
				"subscription must be absent" );
		ensure( !so_has_subscription<msg_1>(so_direct_mbox(), st_dummy_2),
				"subscription must be absent" );

		so_subscribe_self().event< msg_1 >( []{} );

		ensure( so_has_subscription<msg_1>(so_direct_mbox(), so_default_state()),
				"subscription must be present" );
		ensure( so_has_subscription<msg_1>(so_direct_mbox()),
				"subscription must be present" );
		ensure( !so_has_subscription<msg_1>(so_direct_mbox(), st_dummy),
				"subscription must be absent" );
		ensure( !so_has_subscription<msg_1>(so_direct_mbox(), st_dummy_2),
				"subscription must be absent" );

		so_subscribe_self().in(st_dummy).event<msg_1>( []{} );

		ensure( so_has_subscription<msg_1>(so_direct_mbox(), so_default_state()),
				"subscription must be present" );
		ensure( so_has_subscription<msg_1>(so_direct_mbox()),
				"subscription must be present" );
		ensure( so_has_subscription<msg_1>(so_direct_mbox(), st_dummy),
				"subscription must be present" );
		ensure( !so_has_subscription<msg_1>(so_direct_mbox(), st_dummy_2),
				"subscription must be absent" );

		so_subscribe_self().in(st_dummy_2).event<msg_1>( []{} );

		ensure( so_has_subscription<msg_1>(so_direct_mbox(), so_default_state()),
				"subscription must be present" );
		ensure( so_has_subscription<msg_1>(so_direct_mbox()),
				"subscription must be present" );
		ensure( so_has_subscription<msg_1>(so_direct_mbox(), st_dummy),
				"subscription must be present" );
		ensure( so_has_subscription<msg_1>(so_direct_mbox(), st_dummy_2),
				"subscription must be present" );

		so_drop_subscription<msg_1>(so_direct_mbox(), st_dummy);

		ensure( so_has_subscription<msg_1>(so_direct_mbox(), so_default_state()),
				"subscription must be present" );
		ensure( so_has_subscription<msg_1>(so_direct_mbox()),
				"subscription must be present" );
		ensure( !so_has_subscription<msg_1>(so_direct_mbox(), st_dummy),
				"subscription must be absent" );
		ensure( so_has_subscription<msg_1>(so_direct_mbox(), st_dummy_2),
				"subscription must be present" );

		so_drop_subscription_for_all_states<msg_1>(so_direct_mbox());

		ensure( !so_has_subscription<msg_1>(so_direct_mbox(), so_default_state()),
					"subscription must be absent" );
		ensure( !so_has_subscription<msg_1>(so_direct_mbox()),
					"subscription must be absent" );
		ensure( !so_has_subscription<msg_1>(so_direct_mbox(), st_dummy),
			"subscription must be absent" );
		ensure( !so_has_subscription<msg_1>(so_direct_mbox(), st_dummy_2),
			"subscription must be absent" );
	}

	virtual void
	so_evt_start() override
	{
		so_deregister_agent_coop_normally();
	}
};

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				so_5::launch( []( so_5::environment_t & env ) {
						env.introduce_coop( []( so_5::coop_t & coop ) {
								coop.make_agent< a_test_t >();
							} );
					} );
			},
			20,
			"so_has_subscription test" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


/*
 * A simple test for state_t::has_subscription method.
 */

#include <iostream>
#include <map>
#include <exception>
#include <stdexcept>
#include <cstdlib>
#include <thread>
#include <chrono>

#include <so_5/all.hpp>

#include "../subscr_storage_factories.hpp"

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

namespace test
{

class a_test_t : public so_5::agent_t
{
	struct msg_1 : public so_5::signal_t {};

	state_t st_dummy{ this };
	state_t st_dummy_2{ this };

public :
	a_test_t( context_t ctx, so_5::subscription_storage_factory_t factory )
		:	so_5::agent_t( ctx + factory )
	{}

	virtual void
	so_define_agent() override
	{
		ensure( !so_default_state().has_subscription<msg_1>(so_direct_mbox()),
				"subscription must be absent" );
		ensure( !st_dummy.has_subscription<msg_1>(so_direct_mbox()),
				"subscription must be absent" );
		ensure( !st_dummy_2.has_subscription<msg_1>(so_direct_mbox()),
				"subscription must be absent" );

		so_default_state().event( [](mhood_t< msg_1 >){} );

		ensure( so_default_state().has_subscription<msg_1>(so_direct_mbox()),
				"subscription must be present" );
		ensure( !st_dummy.has_subscription<msg_1>(so_direct_mbox()),
				"subscription must be absent" );
		ensure( !st_dummy_2.has_subscription<msg_1>(so_direct_mbox()),
				"subscription must be absent" );

		st_dummy.event( [](mhood_t<msg_1>){} );

		ensure( so_default_state().has_subscription<msg_1>(so_direct_mbox()),
				"subscription must be present" );
		ensure( st_dummy.has_subscription<msg_1>(so_direct_mbox()),
				"subscription must be present" );
		ensure( !st_dummy_2.has_subscription<msg_1>(so_direct_mbox()),
				"subscription must be absent" );

		st_dummy_2.event( [](mhood_t<msg_1>){} );

		ensure( so_default_state().has_subscription<msg_1>(so_direct_mbox()),
				"subscription must be present" );
		ensure( st_dummy.has_subscription<msg_1>(so_direct_mbox()),
				"subscription must be present" );
		ensure( st_dummy_2.has_subscription<msg_1>(so_direct_mbox()),
				"subscription must be present" );

		st_dummy.drop_subscription<msg_1>(so_direct_mbox());

		ensure( so_default_state().has_subscription<msg_1>(so_direct_mbox()),
				"subscription must be present" );
		ensure( !st_dummy.has_subscription<msg_1>(so_direct_mbox()),
				"subscription must be absent" );
		ensure( st_dummy_2.has_subscription<msg_1>(so_direct_mbox()),
				"subscription must be present" );

		so_drop_subscription_for_all_states<msg_1>(so_direct_mbox());

		ensure( !so_default_state().has_subscription<msg_1>(so_direct_mbox()),
					"subscription must be absent" );
		ensure( !st_dummy.has_subscription<msg_1>(so_direct_mbox()),
			"subscription must be absent" );
		ensure( !st_dummy_2.has_subscription<msg_1>(so_direct_mbox()),
			"subscription must be absent" );
	}

	virtual void
	so_evt_start() override
	{
		so_deregister_agent_coop_normally();
	}
};

} /* namespace test */

using namespace test;

int
main()
{
	try
	{
		const auto factories = build_subscr_storage_factories();
		for( const auto & pair : factories )
		{
			const auto & n = pair.first;
			const auto & f = pair.second;
			run_with_time_limit(
				[&]()
				{
					std::cout << n << ": " << std::flush;

					so_5::launch( [f]( so_5::environment_t & env ) {
							env.introduce_coop( [f]( so_5::coop_t & coop ) {
									coop.make_agent< a_test_t >(f);
								} );
						} );

					std::cout << "OK" << std::endl;
				},
				20,
				n );
		}
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


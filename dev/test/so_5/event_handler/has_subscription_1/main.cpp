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

	void
	so_define_agent() override
	{
		ensure( !so_has_subscription<msg_1>(so_direct_mbox(), so_default_state()),
				"(1) subscription must be absent (msg_1, direct, default)" );
		ensure( !so_has_subscription<msg_1>(so_direct_mbox()),
				"(1) subscription must be absent (msg_1, direct)" );
		ensure( !so_has_subscription<msg_1>(so_direct_mbox(), st_dummy),
				"(1) subscription must be absent (msg_1, direct, st_dummy)" );
		ensure( !so_has_subscription<msg_1>(so_direct_mbox(), st_dummy_2),
				"(1) subscription must be absent (msg_1, direct, st_dummy_2)" );

		so_subscribe_self().event( [](mhood_t< msg_1 >){} );

		ensure( so_has_subscription<msg_1>(so_direct_mbox(), so_default_state()),
				"(2) subscription must be present (msg_1, direct, default)" );
		ensure( so_has_subscription<msg_1>(so_direct_mbox()),
				"(2) subscription must be present (msg_1, direct)" );
		ensure( !so_has_subscription<msg_1>(so_direct_mbox(), st_dummy),
				"(2) subscription must be absent (msg_1, direct, st_dummy)" );
		ensure( !so_has_subscription<msg_1>(so_direct_mbox(), st_dummy_2),
				"(2) subscription must be absent (msg_1, direct, st_dummy_2)" );

		so_subscribe_self().in(st_dummy).event( [](mhood_t<msg_1>){} );

		ensure( so_has_subscription<msg_1>(so_direct_mbox(), so_default_state()),
				"(3) subscription must be present (msg_1, direct, default)" );
		ensure( so_has_subscription<msg_1>(so_direct_mbox()),
				"(3) subscription must be present (msg_1, direct)" );
		ensure( so_has_subscription<msg_1>(so_direct_mbox(), st_dummy),
				"(3) subscription must be present (msg_1, direct, st_dummy)" );
		ensure( !so_has_subscription<msg_1>(so_direct_mbox(), st_dummy_2),
				"(3) subscription must be absent (msg_1, direct, st_dummy_2)" );

		so_subscribe_self().in(st_dummy_2).event( [](mhood_t<msg_1>){} );

		ensure( so_has_subscription<msg_1>(so_direct_mbox(), so_default_state()),
				"(4) subscription must be present (msg_1, direct, default)" );
		ensure( so_has_subscription<msg_1>(so_direct_mbox()),
				"(4) subscription must be present (msg_1, direct)" );
		ensure( so_has_subscription<msg_1>(so_direct_mbox(), st_dummy),
				"(4) subscription must be present (msg_1, direct, st_dummy)" );
		ensure( so_has_subscription<msg_1>(so_direct_mbox(), st_dummy_2),
				"(4) subscription must be present (msg_1, direct, st_dummy_2)" );

		so_drop_subscription<msg_1>(so_direct_mbox(), st_dummy);

		ensure( so_has_subscription<msg_1>(so_direct_mbox(), so_default_state()),
				"(5) subscription must be present (msg_1, direct, default)" );
		ensure( so_has_subscription<msg_1>(so_direct_mbox()),
				"(5) subscription must be present (msg_1, direct)" );
		ensure( !so_has_subscription<msg_1>(so_direct_mbox(), st_dummy),
				"(5) subscription must be absent (msg_1, direct, st_dummy)" );
		ensure( so_has_subscription<msg_1>(so_direct_mbox(), st_dummy_2),
				"(5) subscription must be present (msg_1, direct, st_dummy_2)" );

		so_drop_subscription_for_all_states<msg_1>(so_direct_mbox());

		ensure( !so_has_subscription<msg_1>(so_direct_mbox(), so_default_state()),
				"(6) subscription must be absent (msg_1, direct, default)" );
		ensure( !so_has_subscription<msg_1>(so_direct_mbox()),
				"(6) subscription must be absent (msg_1, direct)" );
		ensure( !so_has_subscription<msg_1>(so_direct_mbox(), st_dummy),
				"(6) subscription must be absent (msg_1, direct, st_dummy)" );
		ensure( !so_has_subscription<msg_1>(so_direct_mbox(), st_dummy_2),
				"(6) subscription must be absent (msg_1, direct, st_dummy_2)" );
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
		for( const auto & [n, f] : factories )
		{
			run_with_time_limit(
				[&n, f]()
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


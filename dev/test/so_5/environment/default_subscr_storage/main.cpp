/*
 * A simple test for checking autoshutdown feature of SO Environment.
 */

#include <iostream>
#include <map>
#include <exception>
#include <stdexcept>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <sstream>

#include <so_5/all.hpp>
#include <so_5/impl/subscription_storage_iface.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

class a_test_t : public so_5::agent_t
	{
	public:
		a_test_t( context_t ctx )
			:	so_5::agent_t( std::move(ctx) )
			{}

		a_test_t( context_t ctx, so_5::subscription_storage_factory_t factory )
			:	so_5::agent_t( ctx + factory )
			{}

		void
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
				std::atomic< int > factory_call_counter{};

				so_5::launch(
					[]( so_5::environment_t & env )
					{
						env.register_agent_as_coop(
							// Will use default storage subscription factory.
							env.make_agent< a_test_t >() );

						env.register_agent_as_coop(
							// Will use explicitly specified storage subscription_storage_factory,
							env.make_agent< a_test_t >(
									so_5::flat_set_based_subscription_storage_factory( 1u ) ) );

					},
					[&]( so_5::environment_params_t & params )
					{
						params.default_subscription_storage_factory(
							[&factory_call_counter]() {
								++factory_call_counter;
								return so_5::default_subscription_storage_factory()();
							} );
					} );

				ensure_or_die(
						1 == factory_call_counter.load( std::memory_order_acquire ),
						"custom default subscription storage factory must be called "
						"exactly once" );
			},
			20 );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


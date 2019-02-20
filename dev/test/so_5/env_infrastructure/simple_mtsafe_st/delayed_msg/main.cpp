/*
 * A test for simple_mtsafe_st_env_infastructure with one simple agent
 * and delayed stop message.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

#include <test/3rd_party/utest_helper/helper.hpp>

using namespace std;

int
main()
{
	try
	{
		run_with_time_limit(
			[]() {
				so_5::launch(
					[&]( so_5::environment_t & env ) {
						class actor_t final : public so_5::agent_t
						{
							struct stop {};
						public:
							using so_5::agent_t::agent_t;

							void so_evt_start() override
							{
								so_subscribe_self().event( [this](mhood_t<stop>) {
									so_deregister_agent_coop_normally();
								} );

								so_5::send_delayed< stop >( *this,
										std::chrono::milliseconds(250) );
							}
						};

						env.introduce_coop( []( so_5::coop_t & coop ) {
							coop.make_agent< actor_t >();
						} );
					},
					[]( so_5::environment_params_t & params ) {
						params.infrastructure_factory(
								so_5::env_infrastructures::simple_mtsafe::factory() );
					} );
			},
			5,
			"simple agent with delayed stop message" );
	}
	catch( const exception & ex )
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}


/*
 * A test for simple_not_mtsafe_st_env_infastructure with one simple agent.
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
				bool agent_started = false;
				bool stop_received = false;
				so_5::launch(
					[&]( so_5::environment_t & env ) {
						class actor_t final : public so_5::agent_t
						{
							struct stop {};

							bool & m_agent_started;
							bool & m_stop_received;
						public:
							actor_t(
								context_t ctx,
								bool & agent_started,
								bool & stop_received )
								:	so_5::agent_t{ std::move(ctx) }
								,	m_agent_started{ agent_started }
								,	m_stop_received{ stop_received }
							{}

							void so_evt_start() override
							{
								m_agent_started = true;
								so_subscribe_self().event( [this](mhood_t<stop>) {
									// Do nothing.
									// Shutdown must be initiated anyway.
									m_stop_received = true;
								} );

								so_5::send< stop >( *this );
							}
						};

						env.introduce_coop( [&]( so_5::coop_t & coop ) {
							coop.make_agent< actor_t >(
									std::ref(agent_started),
									std::ref(stop_received) );
						} );
					},
					[]( so_5::environment_params_t & params ) {
						params.infrastructure_factory(
								so_5::env_infrastructures::simple_not_mtsafe::factory() );
					} );

				UT_CHECK_CONDITION( agent_started );
				UT_CHECK_CONDITION( stop_received );
			},
			5,
			"simple agent and stop when no load" );
	}
	catch( const exception & ex )
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}


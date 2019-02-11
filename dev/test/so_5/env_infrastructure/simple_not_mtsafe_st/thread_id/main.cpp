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
				const auto this_thread_id = so_5::query_current_thread_id();
				so_5::current_thread_id_t agent_thread_id;

				so_5::launch(
					[&]( so_5::environment_t & env ) {
						class actor_t final : public so_5::agent_t 
						{
							so_5::current_thread_id_t & m_thread_id;
						public:
							actor_t( context_t ctx, so_5::current_thread_id_t & thread_id )
								:	so_5::agent_t{ std::move(ctx) }
								,	m_thread_id{ thread_id }
							{}

							void so_evt_start() override
							{
								m_thread_id = so_5::query_current_thread_id();
								so_deregister_agent_coop_normally();
							}
						};
						env.introduce_coop( [&]( so_5::coop_t & coop ) {
							coop.make_agent< actor_t >( std::ref(agent_thread_id) );
						} );
					},
					[]( so_5::environment_params_t & params ) {
						params.infrastructure_factory(
								so_5::env_infrastructures::simple_not_mtsafe::factory() );
					} );

				std::cout << "this_thread_id: " << this_thread_id
						<< ", agent_thread_id: " << agent_thread_id
						<< std::endl;

				UT_CHECK_CONDITION( this_thread_id == agent_thread_id );
			},
			5,
			"thread id check" );
	}
	catch( const exception & ex )
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}


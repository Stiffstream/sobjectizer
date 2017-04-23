/*
 * A test for simple_mtsafe_st_env_infastructure with one simple agent
 * and periodic message.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

#include <utest_helper_1/h/helper.hpp>

using namespace std;

class a_test_t final : public so_5::agent_t
{
	int m_ticks{ 0 };
	so_5::timer_id_t m_tick_timer;

	struct tick : public so_5::signal_t {};

public :
	a_test_t( context_t ctx ) : so_5::agent_t( std::move(ctx) )
	{
		so_subscribe_self().event< tick >( [this] {
				++m_ticks;
				if( 3 == m_ticks )
					so_deregister_agent_coop_normally();
			} );
	}

	virtual void so_evt_start() override
	{
		m_tick_timer = so_5::send_periodic< tick >( *this,
				chrono::milliseconds(250),
				chrono::milliseconds(300) );
	}
};

int
main()
{
	try
	{
		run_with_time_limit(
			[]() {
				so_5::launch(
					[&]( so_5::environment_t & env ) {
						env.register_agent_as_coop( so_5::autoname,
								env.make_agent< a_test_t >() );
					},
					[]( so_5::environment_params_t & params ) {
						params.infrastructure_factory(
								so_5::env_infrastructures::simple_not_mtsafe::factory() );
					} );
			},
			5,
			"simple agent with periodic message" );
	}
	catch( const exception & ex )
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}


/*
 * A test for simple_not_mtsafe_st_env_infastructure with turning run-time
 * stats distribution on.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

#include <test/3rd_party/utest_helper/helper.hpp>

using namespace std;

struct run_result_t
{
	int m_first_run_starts{ 0 };
	int m_first_run_stops{ 0 };

	int m_second_run_starts{ 0 };
	int m_second_run_stops{ 0 };
};

class a_test_t final : public so_5::agent_t
{
	so_5::outliving_reference_t< run_result_t > m_result;

	state_t st_first{ this }, st_second{ this };

	struct start_second : public so_5::signal_t {};
	struct finish_second : public so_5::signal_t {};

public :
	a_test_t(
		context_t ctx,
		so_5::outliving_reference_t< run_result_t > result )
		:	so_5::agent_t( std::move(ctx) )
		,	m_result( std::move(result) )
	{}

	virtual void
	so_define_agent() override
	{
		this >>= st_first;

		st_first
			.time_limit( std::chrono::milliseconds( 750 ), st_second )
			.on_exit( [this] {
					so_environment().stats_controller().turn_off();
				} )
			.event( so_environment().stats_controller().mbox(),
				[this]( mhood_t< so_5::stats::messages::distribution_started > ) {
					m_result.get().m_first_run_starts += 1;
				} )
			.event( so_environment().stats_controller().mbox(),
				[this]( mhood_t< so_5::stats::messages::distribution_finished > ) {
					m_result.get().m_first_run_stops += 1;
				} );

		st_second
			.on_enter( [this] {
					so_5::send_delayed< start_second >( *this,
							std::chrono::milliseconds( 400 ) );
				} )
			.event( [this](mhood_t< start_second >) {
					so_environment().stats_controller().turn_on();
					so_5::send_delayed< finish_second >( *this,
							std::chrono::milliseconds( 1050 ) );
				} )
			.event( [this](mhood_t< finish_second >) {
					so_deregister_agent_coop_normally();
				} )
			.event( so_environment().stats_controller().mbox(),
				[this]( mhood_t< so_5::stats::messages::distribution_started > ) {
					m_result.get().m_second_run_starts += 1;
				} )
			.event( so_environment().stats_controller().mbox(),
				[this]( mhood_t< so_5::stats::messages::distribution_finished > ) {
					m_result.get().m_second_run_stops += 1;
				} );
	}

	virtual void
	so_evt_start() override
	{
		auto & controller = so_environment().stats_controller();
		controller.set_distribution_period( std::chrono::milliseconds( 300 ) );
		controller.turn_on();
	}
};

int
main()
{
	try
	{
		run_with_time_limit(
			[]() {
				run_result_t result;
				so_5::launch(
					[&]( so_5::environment_t & env ) {
						env.introduce_coop( [&]( so_5::coop_t & coop ) {
							coop.make_agent< a_test_t >(
									so_5::outliving_mutable(result) );
						} );
					},
					[]( so_5::environment_params_t & params ) {
						params.infrastructure_factory(
								so_5::env_infrastructures::simple_not_mtsafe::factory() );
					} );

				UT_CHECK_GT( result.m_first_run_starts, 1 );
				UT_CHECK_GT( result.m_first_run_stops, 1 );
				UT_CHECK_GT( result.m_second_run_starts, 1 );
				UT_CHECK_GT( result.m_second_run_stops, 1 );
			},
			5,
			"start/stop stats_controller" );
	}
	catch( const exception & ex )
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}


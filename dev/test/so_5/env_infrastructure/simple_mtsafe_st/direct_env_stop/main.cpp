/*
 * A test for direct call to so_environment().stop().
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

#include <utest_helper_1/h/helper.hpp>

using namespace std;

class test_agent : public so_5::agent_t 
{
	struct stop final : public so_5::signal_t {};
public :
	test_agent( context_t ctx ) : so_5::agent_t(std::move(ctx))
	{
		so_subscribe_self().event( &test_agent::on_stop );
	}

	virtual void
	so_evt_start() override
	{
		so_5::send<stop>( *this );
	}

private :
	void
	on_stop( mhood_t<stop> )
	{
		so_environment().stop();
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
						env.register_agent_as_coop(
								so_5::autoname,
								env.make_agent< test_agent >() );
					},
					[]( so_5::environment_params_t & params ) {
						params.infrastructure_factory(
								so_5::env_infrastructures::simple_mtsafe::factory() );
					} );
			},
			5 );
	}
	catch( const exception & ex )
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}


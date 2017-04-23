/*
 * A test for simple_not_mtsafe_st_env_infastructure with
 * autoshutdown_guard.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

#include <utest_helper_1/h/helper.hpp>

using namespace std;

int
main()
{
	try
	{
		run_with_time_limit(
			[]() {
				try {
					so_5::launch( []( so_5::environment_t & ) {},
						[]( so_5::environment_params_t & params ) {
							params.disable_autoshutdown();
							params.infrastructure_factory(
									so_5::env_infrastructures::simple_not_mtsafe::factory() );
						} );

					UT_CHECK_CONDITION( nullptr == "Exception must be thrown" );
				}
				catch( const so_5::exception_t & x )
				{
					UT_CHECK_EQ( so_5::rc_autoshutdown_must_be_enabled,
							x.error_code() );
				}
			},
			5,
			"empty init_fn for simple_not_mtsafe_st_env_infrastructure" );
	}
	catch( const exception & ex )
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}


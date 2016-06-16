/*
 * A simple test for simple prepared select on several mchains.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

#include <utest_helper_1/h/helper.hpp>

#include "../mchain_params.hpp"

using namespace std;

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				struct hello {};

				so_5::wrapped_env_t env;

				auto params = build_mchain_params();

				for( const auto & p : params )
				{
					cout << "=== " << p.first << " ===" << endl;

					auto ch1 = env.environment().create_mchain( p.second );
					auto ch2 = env.environment().create_mchain( p.second );
					auto ch3 = env.environment().create_mchain( p.second );

					bool hello_received = false;

					so_5::send< hello >( ch2 );

					auto r = select( so_5::prepare_select(
							so_5::from_all().extract_n( 1 ).no_wait_on_empty(),
							case_( ch1, []( hello ) {
									throw std::runtime_error( "hello from ch1!" );
								} ),
							case_( ch2, [&hello_received]( hello ) {
									hello_received = true;
								} ),
							case_( ch3, []( hello ) {
									throw std::runtime_error( "hello from ch3!" );
								} )
							) );

					UT_CHECK_CONDITION( 1 == r.extracted() );
					UT_CHECK_CONDITION( 1 == r.handled() );
					UT_CHECK_CONDITION( hello_received );
				}
			},
			20,
			"test for simple multi chain select" );
	}
	catch( const exception & ex )
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}


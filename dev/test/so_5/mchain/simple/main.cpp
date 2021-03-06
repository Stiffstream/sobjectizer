/*
 * A simple test for mchain.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

#include <test/3rd_party/utest_helper/helper.hpp>

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
				struct hello : public so_5::signal_t {};

				so_5::wrapped_env_t env;

				auto params = build_mchain_params();

				for( const auto & p : params )
				{
					cout << "=== " << p.first << " ===" << endl;

					auto chain = env.environment().create_mchain( p.second );

					bool hello_received = false;

					so_5::send< int >( chain, 42 );
					so_5::send< hello >( chain );

					auto r = receive(
							from(chain).handle_n(1).no_wait_on_empty(),
							[]( int i ) {
								if( 42 != i )
									throw runtime_error( "unexpected int-message: "
											+ to_string( i ) );
							},
							[]( const std::string & s ) {
								throw runtime_error( "unexpected string msg: " + s );
							},
							[&hello_received](so_5::mhood_t< hello >) {
								hello_received = true;
							} );

					UT_CHECK_CONDITION( 1 == r.extracted() );
					UT_CHECK_CONDITION( 1 == r.handled() );
					UT_CHECK_CONDITION( !hello_received );
				}
			},
			20,
			"simple test for mchain" );
	}
	catch( const exception & ex )
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}


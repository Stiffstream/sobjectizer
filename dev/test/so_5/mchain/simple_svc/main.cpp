/*
 * A simple test for mchain.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

#include <utest_helper_1/h/helper.hpp>

#include "../mchain_params.hpp"

using namespace std;

void
do_check( const so_5::mchain_t & chain )
{
	auto f1 = so_5::request_future< std::string, int >( chain, 42 );
	auto f2 = so_5::request_future< std::string, int >( chain, -1 );

	auto r = receive(
			from( chain ).handle_n( 2 ),
			[]( int i ) -> std::string {
				if( i < 0 )
					throw invalid_argument( "negative value" );
				return std::to_string( i );
			} );

	UT_CHECK_CONDITION( r.handled() == 2 );

	UT_CHECK_CONDITION( "42" == f1.get() );

	UT_CHECK_THROW( invalid_argument, f2.get() );
}

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				so_5::wrapped_env_t env;

				auto params = build_mchain_params();

				for( const auto & p : params )
				{
					cout << "=== " << p.first << " ===" << endl;

					do_check( env.environment().create_mchain( p.second ) );
				}
			},
			20,
			"simple test for msg_bag" );
	}
	catch( const exception & ex )
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}


/*
 * A simple test for simple select on several closed mchains.
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
				struct second_started {};

				so_5::wrapped_env_t env;

				auto params = build_mchain_params();

				for( const auto & p : params )
				{
					cout << "=== " << p.first << " ===" << endl;

					auto ch1 = env.environment().create_mchain( p.second );
					auto ch2 = env.environment().create_mchain( p.second );
					auto ch3 = env.environment().create_mchain( p.second );

					auto ready = env.environment().create_mchain(
							so_5::make_unlimited_mchain_params() );

					std::thread second{ [ch1, ch2, ch3, ready]{
						so_5::send< second_started >( ready );
						so_5::select( so_5::infinite_wait,
							case_( ch1, [](int) {} ),
							case_( ch2, [](int) {} ),
							case_( ch3, [](int) {} ) );
					} };

					receive( ready, so_5::infinite_wait,
						[&ch1, &ch2, &ch3]( second_started ) {
							std::cout << "second thread started, closing chains..."
									<< std::endl;
							std::this_thread::sleep_for( std::chrono::milliseconds(200) );
							close_retain_content( ch1 );
							close_retain_content( ch2 );
							close_retain_content( ch3 );
						} );

					second.join();
				}
			},
			60,
			"test for simple multi chain select on closed chains" );
	}
	catch( const exception & ex )
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}


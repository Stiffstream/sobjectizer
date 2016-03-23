/*
 * Test for closing mchain which is used in different thread in
 * different selects.
 */

#include <so_5/all.hpp>

#include <string>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

using namespace std;

struct ready {};

void
worker_thread( so_5::mchain_t ready_ch, so_5::mchain_t test_ch )
{
	so_5::send< ready >( ready_ch );
	so_5::select( so_5::from_all(), case_( test_ch, [](int) {} ) );
}

so_5::mchain_params_t
mchain_params()
{
	return so_5::make_unlimited_mchain_params();
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

				auto ready_ch = env.environment().create_mchain( mchain_params() );
				auto test_ch = env.environment().create_mchain( mchain_params() );

				thread w1{ [ready_ch, test_ch]{ worker_thread( ready_ch, test_ch ); } };
				thread w2{ [ready_ch, test_ch]{ worker_thread( ready_ch, test_ch ); } };
				thread w3{ [ready_ch, test_ch]{ worker_thread( ready_ch, test_ch ); } };

				receive( from( ready_ch ).handle_n( 3 ), []( ready ) {} );
				this_thread::sleep_for( chrono::milliseconds(500) );

				close_drop_content( test_ch );

				w1.join();
				w2.join();
				w3.join();
			},
			20,
			"close mchain (adv_select in different threads)" );
	}
	catch( const exception & ex )
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}


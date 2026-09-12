/*
 * A simple test for case with dandling demand in scheduler's queue.
 *
 * NOTE: this test is actual only if this_thread_scheduler_t does a check
 * for emptiness of waiting/ready lists after completion of the top-level
 * task.
 */

#include <so_5/cpp_coro/mchain_select.hpp>
#include <so_5/cpp_coro/this_thread_scheduler.hpp>
#include <so_5/cpp_coro/task.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

#include <test/3rd_party/utest_helper/helper.hpp>

#include <test/so_5/mchain/mchain_params.hpp>

using namespace std;

void
do_test()
{
	struct hello {};
	struct bye {};

	so_5::wrapped_env_t env;

	auto params = build_mchain_params();

	for( const auto & p : params )
	{
		cout << "=== " << p.first << " ===" << endl;

		auto ch1 = env.environment().create_mchain( p.second );
		auto ch2 = env.environment().create_mchain( p.second );

		bool hello_received = false;
		bool bye_received = false;

		so_5::send< hello >( ch1 );

		so_5::cpp_coro::this_thread_scheduler_t scheduler{
				env.environment()
			};

		auto r = scheduler.sync_wait(
				so_5::cpp_coro::select(
						scheduler,
						so_5::from_all().handle_n(1),
						receive_case( ch1, [ch2, &hello_received]( hello ) {
								so_5::send< bye >( ch2 );
								hello_received = true;
							} ),
						receive_case( ch2, [&bye_received]( bye ) {
								bye_received = true;
							} )
						)
				);

		UT_CHECK_CONDITION( 1 == r.extracted() );
		UT_CHECK_CONDITION( 1 == r.handled() );
		UT_CHECK_CONDITION( hello_received );
		UT_CHECK_CONDITION( !bye_received );
	}
}

int
main()
{
	try
	{
		run_with_time_limit(
			do_test,
			5,
			"test for simple multi chain select" );
	}
	catch( const exception & ex )
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}


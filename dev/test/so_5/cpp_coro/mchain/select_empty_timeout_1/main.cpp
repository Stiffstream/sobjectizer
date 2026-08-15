/*
 * Test for empty_timeout.
 */

#include <so_5/cpp_coro/mchain_select.hpp>
#include <so_5/cpp_coro/this_thread_scheduler.hpp>
#include <so_5/cpp_coro/task.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

#include <test/3rd_party/utest_helper/helper.hpp>

#include <test/so_5/mchain/mchain_params.hpp>

using namespace std;

template<typename T> struct debug;

void
do_test()
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

		so_5::cpp_coro::this_thread_scheduler_t scheduler{
				env.environment()
			};

		auto r = scheduler.sync_wait(
				so_5::cpp_coro::select(
						scheduler,
						so_5::from_all()
							.handle_n(1)
							.empty_timeout( std::chrono::milliseconds{ 125 } ),
						receive_case( ch1, []( hello ) {
								throw std::runtime_error( "hello from ch1!" );
							} ),
						receive_case( ch2, [&hello_received]( hello ) {
								hello_received = true;
							} ),
						receive_case( ch3, []( hello ) {
								throw std::runtime_error( "hello from ch3!" );
							} )
						)
				);

		UT_CHECK_CONDITION( !hello_received );
		UT_CHECK_CONDITION( 0 == r.extracted() );
		UT_CHECK_CONDITION( 0 == r.handled() );
	}
}

int
main()
{
	try
	{
		run_with_time_limit(
			do_test,
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


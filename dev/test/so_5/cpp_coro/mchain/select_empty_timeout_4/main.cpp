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

		so_5::cpp_coro::this_thread_scheduler_t scheduler{
				env.environment()
			};

		const auto started_at = std::chrono::steady_clock::now();

		scheduler.sync_wait(
				so_5::cpp_coro::select(
						scheduler,
						so_5::from_all()
							.handle_all()
							.empty_timeout( std::chrono::minutes{ 5 } )
							.total_time( std::chrono::milliseconds{ 100 } ),
						receive_case( ch1, []( hello ) {
								throw std::runtime_error( "hello from ch1!" );
							} ),
						receive_case( ch2, []( hello ) {
								throw std::runtime_error( "hello from ch2!" );
							} ),
						receive_case( ch3, []( hello ) {
								throw std::runtime_error( "hello from ch3!" );
							} )
						)
				);

		const auto duration = std::chrono::steady_clock::now() - started_at;

		UT_CHECK_LT( duration, std::chrono::milliseconds{ 150 } );
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


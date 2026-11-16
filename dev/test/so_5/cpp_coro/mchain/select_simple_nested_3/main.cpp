/*
 * A simple test for simple select on several mchains.
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
	struct bye {};

	so_5::wrapped_env_t env;

	auto params = build_mchain_params();

	for( const auto & p : params )
	{
		cout << "=== " << p.first << " ===" << endl;

		auto ch1 = env.environment().create_mchain( p.second );
		auto ch2 = env.environment().create_mchain( p.second );
		auto ch3 = env.environment().create_mchain( p.second );

		bool hello_received = false;
		bool bye_received = false;

		so_5::send< hello >( ch1 );
		so_5::send_delayed< bye >( ch2, std::chrono::milliseconds{ 20 } );

		so_5::cpp_coro::this_thread_scheduler_t scheduler{
				env.environment()
			};
		so_5::cpp_coro::current_scheduler_setter_t current_scheduler{ scheduler };

		auto r = scheduler.sync_wait(
				so_5::cpp_coro::select(
					so_5::from_all().handle_n(2),
					receive_case( ch1,
						[&]( hello ) -> so_5::cpp_coro::task_t<void>
						{
							co_await so_5::cpp_coro::select(
								so_5::from_all().handle_n(2),
								receive_case( ch2,
									[&]( bye ) -> so_5::cpp_coro::task_t<void>
									{
										bye_received = true;

										so_5::send_delayed< hello >(
												ch3,
												std::chrono::milliseconds{ 20 } );
										so_5::send_delayed< bye >(
												ch2,
												std::chrono::milliseconds{ 40 } );

										co_return;
									} ) );

							hello_received = true;
						} ),
					receive_case( ch3,
						[]( hello ) {} )
					)
				);

		UT_CHECK_CONDITION( 2 == r.extracted() );
		UT_CHECK_CONDITION( 2 == r.handled() );
		UT_CHECK_CONDITION( hello_received );
		UT_CHECK_CONDITION( bye_received );
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


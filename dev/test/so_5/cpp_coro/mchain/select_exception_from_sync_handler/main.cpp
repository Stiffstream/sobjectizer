/*
 * A simple test for exception from a synchronous handler.
 */

#include <so_5/cpp_coro/mchain_select.hpp>
#include <so_5/cpp_coro/this_thread_scheduler.hpp>
#include <so_5/cpp_coro/task.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

#include <test/3rd_party/utest_helper/helper.hpp>

#include <test/so_5/mchain/mchain_params.hpp>

#include <stdexcept>

using namespace std;

template<typename T> struct debug;

void
do_test()
{
	struct hello {};

	class my_exception final : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
	};

	so_5::wrapped_env_t env;

	auto params = build_mchain_params();

	for( const auto & p : params )
	{
		cout << "=== " << p.first << " ===" << endl;

		auto ch1 = env.environment().create_mchain( p.second );

		bool exception_thrown = false;

		so_5::send< hello >( ch1 );

		so_5::cpp_coro::this_thread_scheduler_t scheduler{
				env.environment()
			};

		try
		{
			std::ignore = scheduler.sync_wait(
					so_5::cpp_coro::select(
								scheduler,
								so_5::from_all().handle_all(),
								receive_case( ch1, []( hello ) {
										throw my_exception( "hello from ch1!" );
									} )
							)
					);
		}
		catch( const my_exception & )
		{
			exception_thrown = true;
		}

		UT_CHECK_CONDITION( exception_thrown );
	}
}

int
main()
{
	try
	{
		run_with_time_limit(
			do_test,
			20 );
	}
	catch( const exception & ex )
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}


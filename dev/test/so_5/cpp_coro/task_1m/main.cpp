// Simple test for many tasks and symmetric transfer.

#include <so_5/cpp_coro/this_thread_scheduler.hpp>
#include <so_5/cpp_coro/task.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

#include <test/3rd_party/utest_helper/helper.hpp>

namespace test
{

constexpr int iterations = 987'654;

so_5::cpp_coro::task_t< int >
completes_synchronously(int v)
	{
		co_return (v + 1);
	}

so_5::cpp_coro::task_t< int >
loop_synchronously()
	{
		int r = 0;
		for( int i = 0; i < iterations; ++i )
			{
				r = co_await completes_synchronously(r);
			}

		co_return r;
	}

void
do_check()
	{
		so_5::cpp_coro::this_thread_scheduler_t scheduler{
				so_5::cpp_coro::this_thread_scheduler_t::no_environment
			};
		so_5::cpp_coro::current_scheduler_setter_t scheduler_setter{ scheduler };

		const auto r = scheduler.sync_wait( loop_synchronously() );

		std::cout << r << std::endl;
		ensure_or_die( r == iterations, "unexpected result value" );
	}

} /* namespace test */

using namespace test;

int
main()
{
	try
	{
		run_with_time_limit(
			[]() {
				do_check();
			},
			15 );
	}
	catch(const std::exception & ex)
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


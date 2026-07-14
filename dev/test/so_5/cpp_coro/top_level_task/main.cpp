// Simple test for top_level_task_t type from this_thread_scheduler.

#include <so_5/cpp_coro/this_thread_scheduler.hpp>
#include <so_5/cpp_coro/task.hpp>

#include <so_5/cpp_coro/details/final_awaiter.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

#include <test/3rd_party/utest_helper/helper.hpp>

namespace test
{

using single_int_task_t = so_5::cpp_coro::task_t< int >;

single_int_task_t
get_int_value()
	{
		co_return 42;
	};

void
check_int_case()
	{
		so_5::cpp_coro::this_thread_scheduler_t scheduler;
		auto r = scheduler.sync_wait( get_int_value() );
		ensure_or_die( 42 == r, "42 is expected" );
	}

using single_void_task_t = so_5::cpp_coro::task_t< void >;

single_void_task_t
get_void_value( int & marker )
	{
		marker = 42;
		co_return;
	};

void
check_void_case()
	{
		int marker = 11;
		so_5::cpp_coro::this_thread_scheduler_t scheduler;
		scheduler.sync_wait( get_void_value( marker ) );
		ensure_or_die( 42 == marker, "42 is expected" );
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
				check_int_case();
				check_void_case();
			},
			5 );
	}
	catch(const std::exception & ex)
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


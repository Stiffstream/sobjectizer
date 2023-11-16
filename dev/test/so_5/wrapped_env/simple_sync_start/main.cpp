/*
 * A simple test for sync start of the wrapped_env.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/ensure.hpp>
#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

using namespace std::chrono_literals;

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				std::atomic< int > check_flag{ 0 };

				const auto t1 = std::chrono::steady_clock::now();
				so_5::wrapped_env_t env{
						so_5::wrapped_env_t::wait_init_completion,
						[&]( so_5::environment_t & /*env*/ ) {
							std::this_thread::sleep_for( std::chrono::milliseconds{ 100 } );
							check_flag = 42;
						}
					};

				const auto t2 = std::chrono::steady_clock::now();
				const auto d = std::chrono::duration_cast< std::chrono::milliseconds >( t2 - t1 );

				ensure_or_die( d >= std::chrono::milliseconds{ 100 },
						"the constructor of wrapped_env_t finished too fast" );
				ensure_or_die( 42 == check_flag.load( std::memory_order_acquire ),
						"unexpected value in check_flag!" );
			},
			10 );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


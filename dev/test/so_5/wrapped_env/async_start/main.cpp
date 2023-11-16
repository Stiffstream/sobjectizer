/*
 * A test for async start of the wrapped_env.
 */

#include <so_5/all.hpp>

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
				std::mutex lock;
				lock.lock();

				std::promise< void > init_functor_started;
				so_5::wrapped_env_t env{
						[&]( so_5::environment_t & /*env*/ ) {
							// Inform that init-functor has started.
							init_functor_started.set_value();

							// Try to acquire lock. It can be possible only
							// when the main thread released the lock.
							std::lock_guard< std::mutex > l{ lock };

							std::cout << "Lock acquired in the init-functor" << std::endl;
						}
					};

				init_functor_started.get_future().get();

				// Now we can release the lock.
				lock.unlock();

				std::cout << "Finishing test..." << std::endl;
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


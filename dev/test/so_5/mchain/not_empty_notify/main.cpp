/*
 * Test for custom 'not_empty' notificator for mchain.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

#include <utest_helper_1/h/helper.hpp>

#include "../mchain_params.hpp"

using namespace std;

class event
	{
		std::mutex m;
		std::condition_variable cv;
		bool signaled = { false };

	public :
		void notify_one()
			{
				std::lock_guard< std::mutex > l{ m };
				signaled = true;
				cv.notify_one();
			}

		void wait()
			{
				std::unique_lock< std::mutex > l{ m };
				if( !signaled )
					cv.wait( l, [this]{ return signaled; } );
				signaled = false;
			}
	};

void
do_check(
	so_5::environment_t & env,
	so_5::mchain_params_t params )
{
	event child_started;
	event chain_not_empty;

	params.not_empty_notificator(
			[&chain_not_empty] { chain_not_empty.notify_one(); } );

	auto ch = env.create_mchain( params );

	std::thread child{ [&] {
		child_started.notify_one();
		chain_not_empty.wait();
		
		receive( ch, so_5::infinite_wait,
				[]( int i ) {
					UT_CHECK_CONDITION( i != 0 );
				} );
	} };

	std::cout << "child starting." << std::flush;
	child_started.wait();

	std::cout << "sending." << std::flush;
	so_5::send< int >( ch, 1 );

	std::cout << "joining." << std::flush;
	child.join();

	std::cout << "OK" << std::endl;
}

int
main()
{
	try
	{
		auto params = build_mchain_params();
		for( const auto & p : params )
		{
			cout << "=== " << p.first << " ===" << endl;

			run_with_time_limit(
				[&p]()
				{
					so_5::wrapped_env_t env;

					do_check( env.environment(), p.second );
				},
				20,
				p.first );
		}
	}
	catch( const exception & ex )
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}


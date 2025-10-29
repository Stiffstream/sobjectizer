/*
 * Test for custom 'empty' notificator for mchain.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

#include <test/3rd_party/utest_helper/helper.hpp>

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
	event chain_empty;

	std::atomic< int > how_many_times_called{ 0 };
	params.empty_notificator(
			[&chain_empty, &how_many_times_called] {
				how_many_times_called += 1;
				chain_empty.notify_one();
			} );

	auto ch = env.create_mchain( params );

	std::thread child{ [&] {
		child_started.notify_one();

		so_5::send< int >( ch, 1 );
		receive( from(ch).handle_n(1),
				[]( int i ) {
					UT_CHECK_CONDITION( i != 0 );
				} );
	} };

	std::cout << "child starting." << std::flush;
	child_started.wait();

	std::cout << "waiting for notification." << std::flush;
	chain_empty.wait();

	std::cout << "joining." << std::flush;
	child.join();

	UT_CHECK_CONDITION( 1 == how_many_times_called.load() );

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


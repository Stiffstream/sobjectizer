/*
 * A simple test for mchain.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

#include "../mchain_params.hpp"

using namespace std;

void
do_check( const so_5::mchain_t & chain )
{
	std::mutex m;
	std::condition_variable cv;

	std::unique_lock< std::mutex > lock{ m };

	bool child_started = false;
	std::thread child{ [&] {
		{
			std::lock_guard< std::mutex > l{ m };
			child_started = true;
			cv.notify_one();
		}
		
		auto r = receive( chain, so_5::infinite_wait );
		if( so_5::mchain_props::extraction_status_t::chain_closed != r.status() )
		{
			std::cerr << "unexpected receive status: "
				<< static_cast<int>(r.status()) << std::endl;
			std::abort();
		}
	} };

	std::cout << "child starting." << std::flush;
	cv.wait( lock, [&child_started] { return child_started; } );
	lock.unlock();

	std::cout << "sleeping." << std::flush;
	std::this_thread::sleep_for( std::chrono::seconds(1) );

	std::cout << "closing." << std::flush;
	close_drop_content( chain );

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

					do_check( env.environment().create_mchain( p.second ) );
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


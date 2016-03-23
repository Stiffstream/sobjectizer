/*
 * A simple test for calling receive for one mchain from several threads.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

#include <utest_helper_1/h/helper.hpp>

#include "../mchain_params.hpp"

using namespace std;

void
do_test( so_5::mchain_t & ch )
{
	struct hello {};

	const size_t THREADS_COUNT = 4;

	atomic< int > started_threads{ 0 };

	vector< thread > threads;
	threads.reserve( THREADS_COUNT );

	for( size_t i = 0; i != THREADS_COUNT; ++i )
		threads.emplace_back( thread{ [&ch, &started_threads] {
				++started_threads;
				receive( ch, so_5::infinite_wait, []( hello ){} );
			} } );

	while( THREADS_COUNT != started_threads )
		this_thread::yield();

	for( size_t i = 0; i != THREADS_COUNT; ++i )
		so_5::send< hello >( ch );

	for( size_t i = 0; i != THREADS_COUNT; ++i )
		threads[ i ].join();
}

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				struct hello : public so_5::signal_t {};

				so_5::wrapped_env_t env;

				auto params = build_mchain_params();

				for( const auto & p : params )
				{
					cout << "=== " << p.first << " ===" << endl;

					auto chain = env.environment().create_mchain( p.second );
					do_test( chain );
				}
			},
			20,
			"multithread receive for mchain" );
	}
	catch( const exception & ex )
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}


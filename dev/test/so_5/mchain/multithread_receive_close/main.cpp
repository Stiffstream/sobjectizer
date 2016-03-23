/*
 * A simple test for calling close on mchain when several threads are
 * sleeping on it.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

#include <utest_helper_1/h/helper.hpp>

#include "../mchain_params.hpp"

using namespace std;

void
do_test_case_1( so_5::mchain_t ch )
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

	for( size_t i = 0; i != THREADS_COUNT / 2; ++i )
		so_5::send< hello >( ch );

	close_retain_content( ch );

	for( size_t i = 0; i != THREADS_COUNT; ++i )
		threads[ i ].join();
}

void
do_test_case_2( so_5::mchain_t ch )
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

	close_retain_content( ch );

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

					cout << "case_1: " << flush;
					do_test_case_1( env.environment().create_mchain( p.second ) );
					cout << "OK" << endl;

					cout << "case_2: " << flush;
					do_test_case_2( env.environment().create_mchain( p.second ) );
					cout << "OK" << endl;
				}
			},
			20,
			"multithread receive for closed mchain" );
	}
	catch( const exception & ex )
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}


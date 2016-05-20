/*
 * A simple unit-test for combined_queue_lock_t.
 */

#include <so_5/disp/mpsc_queue_traits/h/pub.hpp>

#include <iostream>
#include <thread>
#include <chrono>
#include <vector>

#include <various_helpers_1/time_limited_execution.hpp>

using namespace so_5::disp::mpsc_queue_traits;

void
test_thread(
	lock_factory_t factory,
	unsigned int milliseconds_to_wait )
{
	lock_unique_ptr_t lock_a{ factory() };
	lock_unique_ptr_t lock_b{ factory() };

	unique_lock_t guard_b{ *lock_b };

	std::thread child{ [&lock_a, &lock_b] {
			unique_lock_t a{ *lock_a };
			{
				// Informs parent that child has started.
				lock_guard_t b{ *lock_b };
				b.notify_one();
			}
			// Try to wait on lock_a.
			a.wait_for_notify();

			{
				// Informs parent that child has started.
				lock_guard_t b{ *lock_b };
				b.notify_one();
			}
		} };

	guard_b.wait_for_notify();
	
	// Acquire lock_a and make child thread to wait for some time.
	{
		lock_guard_t guard_a{ *lock_a };
		std::this_thread::sleep_for(
				std::chrono::milliseconds{ milliseconds_to_wait } );
		guard_a.notify_one();
	}

	// Wait for notify again.
	guard_b.wait_for_notify();

	child.join();
}

void
do_pair_thread_test_case(
	lock_factory_t factory,
	unsigned int milliseconds_to_wait )
{
	std::thread parent{ test_thread, factory, milliseconds_to_wait };
	parent.join();
}

void
single_pair_test_case( lock_factory_t factory )
{
	run_with_time_limit( [factory] {
			std::cout << "single pair: " << std::flush;
			do_pair_thread_test_case( factory, 250u );
			std::cout << "OK" << std::endl;
		},
		20,
		"single_pair_test_case" );

}

void
serie_of_pair_tests_with_equal_intervals( lock_factory_t factory )
{
	std::cout << "serie of pair tests with equal interval: " << std::flush;

	for( unsigned int i = 0; i != 50; ++i )
	{
		run_with_time_limit( [factory] {
			do_pair_thread_test_case( factory, 25u );
		},
		20,
		"serie_of_pair_tests_with_equal_intervals" );
	}

	std::cout << "OK" << std::endl;

}

void
serie_of_pair_tests_with_different_intervals( lock_factory_t factory )
{
	std::cout << "serie of pair tests with different intervals: " << std::flush;

	for( unsigned int i = 1; i != 50; ++i )
	{
		run_with_time_limit( [factory, i] {
			do_pair_thread_test_case( factory, i*5u );
		},
		20,
		"serie_of_pair_tests_with_different_intervals" );
	}

	std::cout << "OK" << std::endl;

}

void
bunch_of_threads( lock_factory_t factory )
{
	std::cout << "bunch of threads: " << std::flush;

	std::vector< std::thread > threads;
	threads.reserve( 50 );

	run_with_time_limit( [&] {
			for( std::size_t i = 0; i != 50; ++i )
				threads.push_back( std::thread{ test_thread, factory, 250u } );

			for( auto & t : threads )
				t.join();
		},
		100,
		"bunch_of_threads" );

	std::cout << "OK" << std::endl;
}

int
main()
{
	try
	{
		struct case_info_t
		{
			std::string m_name;
			lock_factory_t m_factory;
		};

		std::vector< case_info_t > cases;
		cases.push_back( case_info_t{ "combined_lock(default)", combined_lock_factory() } );
		cases.push_back( case_info_t{ "combined_lock(1us)",
				combined_lock_factory( std::chrono::microseconds(1) ) } );
		cases.push_back( case_info_t{ "simple_lock", simple_lock_factory() } );

		for( const auto & c : cases )
		{
			std::cout << "CHECKING CASE: " << c.m_name << "..." << std::endl;

			single_pair_test_case( c.m_factory );
			serie_of_pair_tests_with_equal_intervals( c.m_factory );
			serie_of_pair_tests_with_different_intervals( c.m_factory );
			bunch_of_threads( c.m_factory );

			std::cout << "--- DONE ---" << std::endl;
		}

		return 0;
	}
	catch( const std::exception & x )
	{
		std::cerr << "Exception: " << x.what() << std::endl;
	}

	return 2;
}


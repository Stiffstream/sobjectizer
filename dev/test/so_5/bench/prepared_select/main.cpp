/*
 * A simple benchmark for select() and prepare_select() performance.
 */

#include <iostream>
#include <iterator>
#include <numeric>
#include <chrono>
#include <cstdlib>

#include <so_5/all.hpp>

#include <various_helpers_1/benchmark_helpers.hpp>

so_5::mchain_t
make_mchain( so_5::environment_t & env )
{
	return so_5::create_mchain( env, 2,
			so_5::mchain_props::memory_usage_t::preallocated,
			so_5::mchain_props::overflow_reaction_t::throw_exception );

}

void
raw_select_case( so_5::environment_t & env )
{
	auto ch1 = make_mchain( env );
	auto ch2 = make_mchain( env );
	auto ch3 = make_mchain( env );

	unsigned long long iterations = 0u;
	const unsigned long long max_iterations = 10000u;

	so_5::send< int >( ch1, 0 );

	benchmarker_t bench;
	bench.start();

	while( iterations < max_iterations )
	{
		so_5::select( so_5::no_wait,
				case_( ch1, [&ch2]( int v ) { so_5::send< int >(ch2, v+1); } ),
				case_( ch2, [&ch3]( int v ) { so_5::send< int >(ch3, v+1); } ),
				case_( ch3, [&ch1]( int v ) { so_5::send< int >(ch1, v+1); } ) );
		++iterations;
	}

	bench.finish_and_show_stats( iterations, "raw_select_case" );
}

void
prepared_select_case( so_5::environment_t & env )
{
	auto ch1 = make_mchain( env );
	auto ch2 = make_mchain( env );
	auto ch3 = make_mchain( env );

	unsigned long long iterations = 0u;
	const unsigned long long max_iterations = 10000u;

	auto prepared = so_5::prepare_select(
			so_5::from_all().handle_n( 1 ).no_wait_on_empty(),
			case_( ch1, [&ch2]( int v ) { so_5::send< int >(ch2, v+1); } ),
			case_( ch2, [&ch3]( int v ) { so_5::send< int >(ch3, v+1); } ),
			case_( ch3, [&ch1]( int v ) { so_5::send< int >(ch1, v+1); } ) );

	so_5::send< int >( ch1, 0 );

	benchmarker_t bench;
	bench.start();

	while( iterations < max_iterations )
	{
		select( prepared );
		++iterations;
	}

	bench.finish_and_show_stats( iterations, "prepared_select_case" );
}

int
main()
{
	try
	{
		so_5::launch(
			[]( so_5::environment_t & env )
			{
				raw_select_case( env );
				prepared_select_case( env );
			} );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


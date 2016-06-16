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

const unsigned long long max_iterations = 10000u;

struct one {};
struct two {};
struct three {};

so_5::mchain_t
make_mchain( so_5::environment_t & env )
{
	return so_5::create_mchain( env, 2,
			so_5::mchain_props::memory_usage_t::preallocated,
			so_5::mchain_props::overflow_reaction_t::throw_exception );

}

void
raw_receive_case( so_5::environment_t & env )
{
	auto ch1 = make_mchain( env );

	unsigned long long iterations = 0u;

	so_5::send< one >( ch1 );

	benchmarker_t bench;
	bench.start();

	while( iterations < max_iterations )
	{
		so_5::receive( ch1, so_5::no_wait,
				[&ch1]( one ) { so_5::send< two >( ch1 ); },
				[&ch1]( two ) { so_5::send< three >( ch1 ); },
				[&ch1]( three ) { so_5::send< one >( ch1 ); } );
		++iterations;
	}

	bench.finish_and_show_stats( iterations, "raw_receive_case" );
}

void
prepared_receive_case( so_5::environment_t & env )
{
	auto ch1 = make_mchain( env );

	unsigned long long iterations = 0u;

	const auto prepared = so_5::prepare_receive(
			from( ch1 ).extract_n( 1 ).no_wait_on_empty(),
			[&ch1]( one ) { so_5::send< two >( ch1 ); },
			[&ch1]( two ) { so_5::send< three >( ch1 ); },
			[&ch1]( three ) { so_5::send< one >( ch1 ); } );

	so_5::send< one >( ch1 );

	benchmarker_t bench;
	bench.start();

	while( iterations < max_iterations )
	{
		so_5::receive( prepared );
		++iterations;
	}

	bench.finish_and_show_stats( iterations, "prepared_receive_case" );
}

int
main()
{
	try
	{
		so_5::launch(
			[]( so_5::environment_t & env )
			{
				raw_receive_case( env );
				prepared_receive_case( env );
			} );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


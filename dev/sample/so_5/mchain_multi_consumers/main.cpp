/*
 * An example for demonstration of using receive from several threads.
 */

#include <iostream>
#include <string>

// Main SObjectizer header file.
#include <so_5/all.hpp>

using namespace std;

size_t get_workers_count()
{
	auto c = thread::hardware_concurrency();
	if( c > 2 )
		// Decrement count of thread because one thread will be producer.
		c -= 1;
	else if( 1 == c )
		// We must have at least two consumer threads.
		c = 2;
	return c;
}

void demo()
{
	// Type of message to be received from every consumer thread as a result.
	struct consumer_result
	{
		thread::id m_id;
		size_t m_values_received;
		uint64_t m_sum;
	};

	// A SObjectizer instance.
	so_5::wrapped_env_t sobj;

	// Message chain to be used for values speading between worker threads.
	auto values_ch = so_5::create_mchain( sobj,
			// Chain is size-limited with blocking of sender on overload.
			// Wait on overloaded mchain for 5min.
			chrono::minutes{5},
			// No more than 300 messages in chain.
			300u,
			// Space for mchain will be preallocated.
			so_5::mchain_props::memory_usage_t::preallocated,
			// What to do on overflow.
			// This value has no sence because we use too large time limit.
			// Because of that use hardest case.
			so_5::mchain_props::overflow_reaction_t::abort_app );

	// Message chain to be used for results from consumers.
	// A very simple chain will be created for that.
	auto results_ch = create_mchain( sobj );

	// Create workers.
	const auto workers_count = get_workers_count();
	vector< thread > workers;
	workers.reserve( workers_count );

	for( size_t i = 0; i != workers_count; ++i )
		workers.emplace_back( thread{ [&values_ch, &results_ch] {
			// Receive all data from input chain.
			size_t received = 0u;
			uint64_t sum = 0u;
			receive( from( values_ch ),
				[&sum, &received]( unsigned int v ) {
					++received;
					sum += v;
				} );

			// Send result back.
			so_5::send< consumer_result >( results_ch,
					this_thread::get_id(), received, sum );
		} } );

	cout << "Workers created: " << workers_count << endl;

	// Send a bunch of values for consumers.
	for( unsigned int i = 0; i != 10000; ++i )
		so_5::send< unsigned int >( values_ch, i );

	// No more values will be sent.
	close_retain_content( values_ch );

	// Receive responses from consumers.
	receive(
		// Exactly workers_count results expected.
		from( results_ch ).handle_n( workers_count ),
		[]( const consumer_result & r ) {
			cout << "Thread: " << r.m_id
					<< ", values: " << r.m_values_received
					<< ", sum: " << r.m_sum
					<< endl;
		} );

	// All consumer threads must be finished.
	for_each( begin(workers), end(workers), []( thread & t ) { t.join(); } );

	// SObjectizer will be stopped automatically.
}

int main()
{
	try
	{
		demo();
	}
	catch( const exception & ex )
	{
		cerr << "Error: " << ex.what() << endl;
	}

	return 0;
}


/*
 * Test for empty_timeout on adv_select.
 */

#include <so_5/all.hpp>

#include <string>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

#include "../mchain_params.hpp"

using namespace std;

struct worker_result
{
	int m_id;
	unsigned int m_values;
};

template< typename DURATION >
void
worker_thread(
	int id,
	so_5::mchain_t control_ch,
	so_5::mchain_t test_ch,
	DURATION empty_timeout )
{
	worker_result r{ id, 0u };

	select(
			so_5::from_all()
					.handle_n( 5 )
					.empty_timeout( empty_timeout ),
			case_( test_ch, [&r]( unsigned int ) { ++r.m_values; } ) );

	so_5::send< worker_result >( control_ch, r );
}

so_5::mchain_params_t
mchain_params()
{
	return so_5::make_unlimited_mchain_params();
}

template< typename DURATION >
void
do_test(
	const char * test_case,
	DURATION empty_timeout,
	DURATION min_time )
{
	std::cout << "starting " << test_case << "..." << std::endl;

	run_with_time_limit(
		[empty_timeout, min_time]()
		{
			so_5::wrapped_env_t env;

			auto control_ch = env.environment().create_mchain( mchain_params() );
			auto test_ch = env.environment().create_mchain( mchain_params() );

			const std::size_t N = 3;

			array< thread, N > workers;
			for( std::size_t i = 0; i != N; ++i )
				workers[ i ] = thread{ [i, control_ch, test_ch, empty_timeout] {
						worker_thread( static_cast< int >(i),
								control_ch, test_ch, empty_timeout );
					} };

			const auto start_point = chrono::steady_clock::now();

			receive( from( control_ch ).handle_n( N ),
					[]( const worker_result & r ) {
						cout << r.m_id << " -> " << r.m_values << std::endl;
						ensure_or_die( r.m_values == 0, "no values must be received" );
					} );

			const auto elapsed = chrono::steady_clock::now() - start_point;
			ensure_or_die( min_time <= elapsed, "min_time must be less than elapsed time" );
			for( auto & w : workers )
				w.join();
		},
		20,
		"read mchain (select in different threads)" );

	std::cout << test_case << "-> OK" << std::endl;
}

int
main()
{
	try
	{
		do_test( "zero_empty_time",
				chrono::milliseconds::zero(),
				chrono::milliseconds::zero() );
		do_test( "1s_empty_time",
				chrono::milliseconds(1000),
				chrono::milliseconds(750) );
	}
	catch( const exception & ex )
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}


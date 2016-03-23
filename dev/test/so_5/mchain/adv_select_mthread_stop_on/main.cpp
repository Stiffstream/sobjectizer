/*
 * Test for stop_on on adv_select.
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

void
worker_thread(
	int id,
	so_5::mchain_t control_ch,
	so_5::mchain_t test_ch )
{
	worker_result r{ id, 0u };

	bool odd_found = false;

	select(
			so_5::from_all()
					.stop_on( [&odd_found]{ return odd_found; } ),
			case_( test_ch,
					[&r, &odd_found]( unsigned int v ) {
						++r.m_values;
						odd_found = 0 != (v & 1);
					} ) );

	so_5::send< worker_result >( control_ch, r );
}

so_5::mchain_params_t
mchain_params()
{
	return so_5::make_unlimited_mchain_params();
}

void
do_test(
	const char * test_case )
{
	std::cout << "starting " << test_case << "..." << std::endl;

	run_with_time_limit(
		[]()
		{
			so_5::wrapped_env_t env;

			auto control_ch = env.environment().create_mchain( mchain_params() );
			auto test_ch = env.environment().create_mchain( mchain_params() );

			const std::size_t N = 3;

			array< thread, N > workers;
			for( std::size_t i = 0; i != N; ++i )
				workers[ i ] = thread{ [i, control_ch, test_ch] {
						worker_thread( static_cast< int >(i), control_ch, test_ch );
					} };

			for( unsigned int i = 0; i != 10; ++i )
				so_5::send< unsigned int >( test_ch, i );

			receive( from( control_ch ).handle_n( N ),
					[]( const worker_result & r ) {
						cout << r.m_id << " -> " << r.m_values << std::endl;
						ensure_or_die( r.m_values > 0, "at least one value must be received" );
					} );

			for( auto & w : workers )
				w.join();
		},
		20,
		"stop_on on adv_select (select in different threads)" );

	std::cout << test_case << "-> OK" << std::endl;
}

int
main()
{
	try
	{
		do_test( "multi-thread adv_select with stop_on" );
	}
	catch( const exception & ex )
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}


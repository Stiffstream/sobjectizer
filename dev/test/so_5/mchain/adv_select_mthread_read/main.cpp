/*
 * Test for reading from chain from different threads.
 */

#include <so_5/all.hpp>

#include <string>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

#include "../mchain_params.hpp"

using namespace std;

struct ready {};
struct done {};
struct worker_result
{
	int m_id;
	unsigned int m_values;
	unsigned int m_sum;
};

template< typename DURATION >
void
worker_thread(
	int id,
	so_5::mchain_t control_ch,
	so_5::mchain_t test_ch,
	DURATION total_time )
{
	worker_result r{ id, 0u, 0u };

	so_5::send< ready >( control_ch );

	bool finish = false;
	select(
			so_5::from_all()
					.handle_n( 5 )
					.total_time( total_time )
					.stop_on( [&finish]{ return finish; } ),
			case_( test_ch,
					[&r]( unsigned int v ) {
						++r.m_values;
						r.m_sum += v;
					},
					[&finish]( done ) {
						finish = true;
					} ) );

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
	DURATION total_time )
{
	std::cout << "starting " << test_case << "..." << std::endl;

	run_with_time_limit(
		[total_time]()
		{
			so_5::wrapped_env_t env;

			auto control_ch = env.environment().create_mchain( mchain_params() );
			auto test_ch = env.environment().create_mchain( mchain_params() );

			thread w1{ [control_ch, test_ch, total_time]{
					worker_thread( 0, control_ch, test_ch, total_time );
			} };
			thread w2{ [control_ch, test_ch, total_time]{
					worker_thread( 1, control_ch, test_ch, total_time );
			} };
			thread w3{ [control_ch, test_ch, total_time]{
					worker_thread( 3, control_ch, test_ch, total_time );
			} };

			receive( from( control_ch ).handle_n( 3 ), []( ready ) {} );

			for( unsigned int i = 0; i != 1000; ++i )
				so_5::send< unsigned int >( test_ch, i );

			so_5::send< done >( test_ch );
			so_5::send< done >( test_ch );
			so_5::send< done >( test_ch );

			receive( from( control_ch ).handle_n( 3 ),
					[]( const worker_result & r ) {
						cout << r.m_id << " -> " << r.m_values
								<< " {" << r.m_sum << "}" << endl;
						ensure_or_die( r.m_values == 5, "only five values must be received" );
					} );

			w1.join();
			w2.join();
			w3.join();
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
		do_test( "infinite_total_time", so_5::infinite_wait );
		do_test( "finite_total_time(5s)", chrono::seconds(5) );
	}
	catch( const exception & ex )
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}


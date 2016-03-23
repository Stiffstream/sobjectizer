/*
 * A simple test for simple select on several mchains.
 */

#include <so_5/all.hpp>

#include <string>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

#include "../mchain_params.hpp"

using namespace std;

void
worker_thread( so_5::mchain_t ch, unsigned int values )
{
	for( unsigned int i = 0; i != values; ++i )
	{
		so_5::send< unsigned int >( ch, i );
	}

	close_retain_content( ch );
}

void
try_accept_value( const char * marker,
	unsigned int value,
	unsigned int & counter,
	unsigned int & expected_value )
{
	if( value != expected_value )
		throw runtime_error( marker + string( ": unexpected value, actual: " ) +
				to_string( value ) + ", expected: " + to_string( expected_value ) );

	expected_value += 1;
	counter += 1;
}

so_5::mchain_params_t
mchain_params()
{
	return so_5::make_limited_with_waiting_mchain_params(
			10,
			so_5::mchain_props::memory_usage_t::dynamic,
			so_5::mchain_props::overflow_reaction_t::drop_newest,
			chrono::seconds(2) );
}

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				so_5::wrapped_env_t env;

				vector< unsigned int > counters = { 0, 0, 0 };
				vector< unsigned int > expected_values = { 0, 0, 0 };

				auto ch1 = env.environment().create_mchain( mchain_params() );
				auto ch2 = env.environment().create_mchain( mchain_params() );
				auto ch3 = env.environment().create_mchain( mchain_params() );

				thread w1{ [ch1]{ worker_thread( ch1, 20 ); } };
				thread w2{ [ch2]{ worker_thread( ch2, 30 ); } };
				thread w3{ [ch3]{ worker_thread( ch3, 40 ); } };

				unsigned int total_values = 0;

				while( true )
				{
					auto r = so_5::select( so_5::infinite_wait,
							case_( ch1, [&]( unsigned int v ) {
									try_accept_value(
											"ch1",
											v,
											counters[ 0 ],
											expected_values[ 0 ] );
									++total_values;
								} ),
							case_( ch2, [&]( unsigned int v ) {
									try_accept_value(
											"ch2",
											v,
											counters[ 1 ],
											expected_values[ 1 ] );
									++total_values;
								} ),
							case_( ch3, [&]( unsigned int v ) {
									try_accept_value(
											"ch3",
											v,
											counters[ 2 ],
											expected_values[ 2 ] );
									++total_values;
								} )
							);

					if( so_5::mchain_props::extraction_status_t::chain_closed ==
							r.status() )
						break;
				}

				ensure_or_die( 90 == total_values, "total values must be 90" );
				ensure_or_die( 20 == counters[ 0 ], "20 messages from ch1" );
				ensure_or_die( 30 == counters[ 1 ], "30 messages from ch2" );
				ensure_or_die( 40 == counters[ 2 ], "40 messages from ch3" );

				w1.join();
				w2.join();
				w3.join();
			},
			20,
			"count messages" );
	}
	catch( const exception & ex )
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}


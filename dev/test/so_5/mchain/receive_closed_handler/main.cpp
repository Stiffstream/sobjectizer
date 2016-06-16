/*
 * A simple test for closed_handler on receive operation.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

#include "../mchain_params.hpp"

using namespace std;

void
check_op(
	const so_5::mchain_t & ch1,
	const so_5::mchain_t & ch2 )
{
	thread second_thread( [&] {
			bool must_continue = true;
			do
			{
				receive(
					from( ch1 ).on_close(
						[&must_continue]( const so_5::mchain_t & ) {
							must_continue = false;
						} ),
					[&ch2]( int v ) {
						so_5::send< int >( ch2, v+1 );
					} );
			}
			while( must_continue );
		} );
	auto second_thread_joiner = so_5::auto_join( second_thread );

	so_5::send< int >( ch1, 0 );
	receive( from( ch2 ).handle_n(3),
		[&ch1]( int v ) {
			cout << "received: " << v << endl;
			so_5::send< int >( ch1, v+1 );
		} );

	close_retain_content( ch1 );
}

void
do_check( bool msg_tracing_enabled )
{
	run_with_time_limit(
		[msg_tracing_enabled]()
		{
			so_5::wrapped_env_t env{
				[]( so_5::environment_t & ) {},
				[msg_tracing_enabled]( so_5::environment_params_t & params ) {
					if( msg_tracing_enabled )
						params.message_delivery_tracer(
								so_5::msg_tracing::std_clog_tracer() );
				} };

			auto params = build_mchain_params();

			for( const auto & p : params )
			{
				cout << "=== " << p.first << " ===" << endl;

				check_op(
						env.environment().create_mchain( p.second ),
						env.environment().create_mchain( p.second ) );
			}
		},
		20,
		"close_chain" );
}

int
main()
{
	try
	{
		do_check( false );
	}
	catch( const exception & ex )
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}


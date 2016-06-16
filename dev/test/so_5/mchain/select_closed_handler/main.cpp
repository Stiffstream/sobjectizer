/*
 * A simple test for closed_handler on select operation.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

#include "../mchain_params.hpp"

using namespace std;

void
check_op(
	const so_5::mchain_t & ch1,
	const so_5::mchain_t & ch2,
	const so_5::mchain_t & ch3 )
{
	thread second_thread( [&] {
			bool must_continue = true;
			size_t chains_closed = 0;
			do
			{
				so_5::select(
					so_5::from_all().on_close(
						[&must_continue, &chains_closed]( const so_5::mchain_t & ch ) {
							++chains_closed;
							cout << "[" << ch->id() << "] closed ("
									<< chains_closed << ")" << endl;
							if( 2 == chains_closed )
								must_continue = false;
						} ),
					case_( ch1, [&ch1]( int v ) {
							cout << "[" << ch1->id() << "]: received " << v << endl;
						} ),
					case_( ch2, [&ch2]( int v ) {
							cout << "[" << ch2->id() << "]: received " << v << endl;
						} ) );
			}
			while( must_continue );

			so_5::send< size_t >( ch3, chains_closed );
		} );
	auto second_thread_joiner = so_5::auto_join( second_thread );

	so_5::send< int >( ch1, 1 );
	so_5::send< int >( ch2, 2 );

	close_retain_content( ch1 );
	close_retain_content( ch2 );

	receive( ch3, so_5::infinite_wait, []( size_t chains_closed ) {
			cout << "total chains closed: " << chains_closed << endl;
			ensure_or_die( 2u == chains_closed,
					"expected value for chains_closed is 2" );
		} );
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
						env.environment().create_mchain( p.second ),
						env.environment().create_mchain( p.second ) );
			}
		},
		20,
		"do_check" );
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


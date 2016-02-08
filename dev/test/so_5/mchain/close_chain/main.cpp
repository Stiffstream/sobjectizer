/*
 * A simple test for mchain.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

#include "../mchain_params.hpp"

using namespace std;

void
check_drop_content( const so_5::mchain_t & chain )
{
	so_5::send< int >( chain, 0 );
	so_5::send< int >( chain, 1 );

	close_drop_content( chain );

	auto r = receive(
			chain,
			so_5::no_wait,
			[]( int i ) {
				throw runtime_error( "unexpected message: " + to_string(i) );
			} );

	if( r.status() != so_5::mchain_props::extraction_status_t::chain_closed )
		throw runtime_error( "unexpected value of so_5::receive "
				"return code: " + to_string( static_cast<int>(r.status()) ) );
}

void
check_retain_content( const so_5::mchain_t & chain )
{
	so_5::send< int >( chain, 0 );
	so_5::send< int >( chain, 1 );

	close_retain_content( chain );

	so_5::mchain_receive_result_t r;
	r = receive(
			chain,
			so_5::no_wait,
			[]( int i ) {
				if( i != 0 )
					throw runtime_error( "unexpected message: " + to_string(i) );
			} );

	if( r.handled() != 1 )
		throw runtime_error( "1: unexpected value of so_5::receive "
				"return code: " + to_string( r.handled() ) );

	r = receive(
			chain,
			so_5::no_wait,
			[]( int i ) {
				if( i != 1 )
					throw runtime_error( "unexpected message: " + to_string(i) );
			} );

	if( r.handled() != 1 )
		throw runtime_error( "2: unexpected value of so_5::receive "
				"return code: " + to_string( r.handled() ) );
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

				check_drop_content(
						env.environment().create_mchain( p.second ) );
				check_retain_content(
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
		do_check( true );
	}
	catch( const exception & ex )
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}


/*
 * Test for checking delayed and periodic messages delivery to mchains.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

#include <utest_helper_1/h/helper.hpp>

#include "../mchain_params.hpp"

using namespace std;

const chrono::milliseconds timeout_delta{ 20 };

template< typename L >
void check_pause(
	const chrono::milliseconds wait_timeout,
	L lambda )
{
	auto s = chrono::steady_clock::now();
	lambda();
	auto f = chrono::steady_clock::now();

	UT_CHECK_CONDITION( s + (wait_timeout - timeout_delta) < f );
}

void
check_delayed( const so_5::mchain_t & chain )
{
	cout << "check_delayed..." << endl;
	check_pause(
			chrono::milliseconds(100),
			[&] {
				so_5::send_delayed< int >( chain, chrono::milliseconds(100), 1 );
				receive( chain, so_5::infinite_wait,
						[]( int i ) { UT_CHECK_CONDITION( i == 1 ); } );
			} );
}

void
check_periodic( const so_5::mchain_t & chain )
{
	cout << "check_periodic..." << endl;
	check_pause(
			chrono::milliseconds(400),
			[&] {
				auto t = so_5::send_periodic< int >(
						chain,
						chrono::milliseconds(100),
						chrono::milliseconds(100),
						1 );
				receive( from(chain).handle_n(4),
						[]( int i ) { UT_CHECK_CONDITION( i == 1 ); } );
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

				check_delayed(
						env.environment().create_mchain( p.second ) );
				check_periodic(
						env.environment().create_mchain( p.second ) );
			}
		},
		20,
		"timers" );
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


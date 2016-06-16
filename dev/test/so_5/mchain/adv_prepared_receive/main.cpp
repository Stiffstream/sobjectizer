/*
 * A simple test for prepared advanced receive.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

#include <utest_helper_1/h/helper.hpp>

#include "../mchain_params.hpp"

using namespace std;
using namespace chrono;

void
do_check_timeout_on_empty_queue( const so_5::mchain_t & chain )
{
	std::thread child{ [&] {
		auto r = receive( prepare_receive( 
				from( chain ).empty_timeout( milliseconds( 500 ) ) ) );

		UT_CHECK_CONDITION( 0 == r.extracted() );
		UT_CHECK_CONDITION(
				so_5::mchain_props::extraction_status_t::no_messages == r.status() );
	} };

	child.join();
}

UT_UNIT_TEST( test_timeout_on_empty_queue )
{
	auto params = build_mchain_params();
	for( const auto & p : params )
	{
		cout << "=== " << p.first << " ===" << endl;

		run_with_time_limit(
			[&p]()
			{
				so_5::wrapped_env_t env;

				do_check_timeout_on_empty_queue(
						env.environment().create_mchain( p.second ) );
			},
			20,
			"test_timeout_on_empty_queue: " + p.first );
	}
}

void
do_check_total_time( const so_5::mchain_t & chain )
{
	so_5::send< int >( chain, 0 );
	so_5::send< int >( chain, 1 );
	so_5::send< string >( chain, "hello!" );

	std::thread child{ [&] {
		auto r = receive( prepare_receive(
				from( chain ).total_time( milliseconds( 500 ) ),
				[]( const std::string & ) {} ) );

		UT_CHECK_CONDITION( 3 == r.extracted() );
		UT_CHECK_CONDITION( 1 == r.handled() );
		UT_CHECK_CONDITION(
				so_5::mchain_props::extraction_status_t::msg_extracted == r.status() );
	} };

	child.join();
}

UT_UNIT_TEST( test_total_time )
{
	auto params = build_mchain_params();
	for( const auto & p : params )
	{
		cout << "=== " << p.first << " ===" << endl;

		run_with_time_limit(
			[&p]()
			{
				so_5::wrapped_env_t env;

				do_check_total_time(
						env.environment().create_mchain( p.second ) );
			},
			20,
			"test_total_time: " + p.first );
	}
}

void
do_check_handle_n(
	const so_5::mchain_t & ch1,
	const so_5::mchain_t & ch2 )
{
	std::thread child{ [&] {
		auto r = receive( prepare_receive( 
				from( ch1 ).handle_n( 3 ),
				[&ch2]( int i ) {
					so_5::send< int >( ch2, i );
				} ) );

		UT_CHECK_CONDITION( 3 == r.extracted() );
		UT_CHECK_CONDITION( 3 == r.handled() );
	} };

	so_5::send< int >( ch1, 0 );
	auto r = receive(
			from( ch2 ).handle_n( 2 ),
			[&ch1]( int i ) {
				so_5::send< int >( ch1, i + 1 );
			} );

	UT_CHECK_CONDITION( 2 == r.extracted() );
	UT_CHECK_CONDITION( 2 == r.handled() );

	child.join();
}

UT_UNIT_TEST( test_handle_n )
{
	auto params = build_mchain_params();
	for( const auto & p : params )
	{
		cout << "=== " << p.first << " ===" << endl;

		run_with_time_limit(
			[&p]()
			{
				so_5::wrapped_env_t env;

				do_check_handle_n(
						env.environment().create_mchain( p.second ),
						env.environment().create_mchain( p.second ) );
			},
			20,
			"test_handle_n: " + p.first );
	}
}

void
do_check_extract_n(
	const so_5::mchain_t & ch1,
	const so_5::mchain_t & ch2 )
{
	std::thread child{ [&] {
		auto r = receive( prepare_receive( 
				from( ch1 ).handle_n( 3 ).extract_n( 3 ),
				[&ch2]( int i ) {
					so_5::send< int >( ch2, i );
				} ) );

		UT_CHECK_CONDITION( 3 == r.extracted() );
		UT_CHECK_CONDITION( 1 == r.handled() );
	} };

	so_5::send< string >( ch1, "0" );
	so_5::send< int >( ch1, 0 );

	auto r = receive( prepare_receive(
			from( ch2 ).handle_n( 1 ),
			[&ch1]( int i ) {
				so_5::send< string >( ch1, to_string( i + 1 ) );
				so_5::send< int >( ch1, i + 1 );
			} ) );

	UT_CHECK_CONDITION( 1 == r.extracted() );
	UT_CHECK_CONDITION( 1 == r.handled() );

	child.join();
}

UT_UNIT_TEST( test_extract_n )
{
	auto params = build_mchain_params();
	for( const auto & p : params )
	{
		cout << "=== " << p.first << " ===" << endl;

		run_with_time_limit(
			[&p]()
			{
				so_5::wrapped_env_t env;

				do_check_extract_n(
						env.environment().create_mchain( p.second ),
						env.environment().create_mchain( p.second ) );
			},
			20,
			"test_extract_n: " + p.first );
	}
}

void
do_check_stop_pred(
	const so_5::mchain_t & ch1,
	const so_5::mchain_t & ch2 )
{
	std::thread child{ [&] {
		int last_received = 0;
		auto r = receive( prepare_receive(
				from( ch1 ).stop_on(
						[&last_received] { return last_received > 10; } ),
				[&ch2, &last_received]( int i ) {
					last_received = i;
					so_5::send< int >( ch2, i );
				} ) );

		UT_CHECK_CONDITION( r.extracted() > 10 );
		UT_CHECK_CONDITION( r.handled() > 10 );
	} };

	int i = 0;
	so_5::send< int >( ch1, i );
	auto r = receive( prepare_receive(
			from( ch2 ).stop_on( [&i] { return i > 10; } ),
			[&ch1, &i]( int ) {
				++i;
				so_5::send< int >( ch1, i );
			} ) );

	UT_CHECK_CONDITION( r.extracted() > 10 );
	UT_CHECK_CONDITION( r.handled() > 10 );

	child.join();
}

UT_UNIT_TEST( test_stop_pred )
{
	auto params = build_mchain_params();
	for( const auto & p : params )
	{
		cout << "=== " << p.first << " ===" << endl;

		run_with_time_limit(
			[&p]()
			{
				so_5::wrapped_env_t env;

				do_check_stop_pred(
						env.environment().create_mchain( p.second ),
						env.environment().create_mchain( p.second ) );
			},
			20,
			"test_stop_pred(no_msg_tracing): " + p.first );

		run_with_time_limit(
			[&p]()
			{
				so_5::wrapped_env_t env(
						[]( so_5::environment_t & ) {},
						[]( so_5::environment_params_t & env_params ) {
							env_params.message_delivery_tracer(
									so_5::msg_tracing::std_clog_tracer() );
						} );

				do_check_stop_pred(
						env.environment().create_mchain( p.second ),
						env.environment().create_mchain( p.second ) );
			},
			20,
			"test_stop_pred(msg_tracing): " + p.first );
	}
}
int
main()
{
	UT_RUN_UNIT_TEST( test_timeout_on_empty_queue )
	UT_RUN_UNIT_TEST( test_total_time )
	UT_RUN_UNIT_TEST( test_handle_n )
	UT_RUN_UNIT_TEST( test_extract_n )
	UT_RUN_UNIT_TEST( test_stop_pred )

	return 0;
}


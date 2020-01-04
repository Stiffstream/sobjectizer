/*
 * An advanced test for extensible select.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

#include <test/3rd_party/utest_helper/helper.hpp>

#include "../mchain_params.hpp"

using namespace std;
using namespace chrono;
using namespace chrono_literals;

void
do_check_timeout_on_empty_queue( const so_5::mchain_t & chain )
{
	std::thread child{ [&] {
		auto sel = prepare_select(
				so_5::from_all().handle_all().empty_timeout( 500ms ),
				receive_case( chain ) );

		auto r = select( sel );

		UT_CHECK_CONDITION( 0 == r.extracted() );
		UT_CHECK_CONDITION( r.is_nothing_happened() );
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
		auto r = select(
				prepare_select(
						so_5::from_all().handle_all().total_time( 500ms ),
						receive_case( chain, []( const std::string & ) {} ) )
				);

		UT_CHECK_CONDITION( 3 == r.extracted() );
		UT_CHECK_CONDITION( 1 == r.handled() );
		UT_CHECK_CONDITION( r.was_handled() );
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
		auto r = select(
				prepare_select( 
						so_5::from_all().handle_n( 3 ),
						receive_case( ch1,
							[&ch2]( int i ) {
								so_5::send< int >( ch2, i );
							} ) )
				);

		UT_CHECK_CONDITION( 3 == r.extracted() );
		UT_CHECK_CONDITION( 3 == r.handled() );
	} };

	so_5::send< int >( ch1, 0 );
	auto r = select(
			prepare_select(
					so_5::from_all().handle_n( 2 ),
					receive_case( ch2,
						[&ch1]( int i ) {
							so_5::send< int >( ch1, i + 1 );
						} ) )
			);

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
		auto r = select(
				prepare_select( 
						so_5::from_all().handle_n( 3 ).extract_n( 3 ),
						receive_case( ch1,
							[&ch2]( int i ) {
								so_5::send< int >( ch2, i );
							} ) )
				);

		UT_CHECK_CONDITION( 3 == r.extracted() );
		UT_CHECK_CONDITION( 1 == r.handled() );
	} };

	so_5::send< string >( ch1, "0" );
	so_5::send< int >( ch1, 0 );

	auto r = select(
			prepare_select(
					so_5::from_all().handle_n( 1 ),
					receive_case( ch2,
						[&ch1]( int i ) {
							so_5::send< string >( ch1, to_string( i + 1 ) );
							so_5::send< int >( ch1, i + 1 );
						} ) )
			);

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
		auto r = select( prepare_select(
				so_5::from_all().handle_all().stop_on(
						[&last_received] { return last_received > 10; } ),
				receive_case( ch1,
					[&ch2, &last_received]( int i ) {
						last_received = i;
						so_5::send< int >( ch2, i );
					} ) )
		);

		UT_CHECK_CONDITION( r.extracted() > 10 );
		UT_CHECK_CONDITION( r.handled() > 10 );
	} };

	int i = 0;
	so_5::send< int >( ch1, i );
	auto r = select( prepare_select(
			so_5::from_all().handle_all().stop_on( [&i] { return i > 10; } ),
			receive_case( ch2,
				[&ch1, &i]( int ) {
					++i;
					so_5::send< int >( ch1, i );
				} ) )
	);

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

void
do_check_parallel_select(
	const so_5::mchain_t & ch,
	const so_5::mchain_t & failure_ch )
{
	struct nothing {};

	auto sel = prepare_select(
			so_5::from_all().handle_all(),
			receive_case( ch, [](const nothing&) {} ) );

	std::thread child1{ [&] {
		try {
			select( sel );
		}
		catch( const so_5::exception_t & x ) {
			so_5::send<int>( failure_ch, x.error_code() );
		}
	} };

	std::thread child2{ [&] {
		try {
			select( sel );
		}
		catch( const so_5::exception_t & x ) {
			so_5::send<int>( failure_ch, x.error_code() );
		}
	} };

	std::optional<int> error;
	so_5::receive(
			so_5::from( failure_ch ).handle_n(1).empty_timeout(500ms),
			[&error](int error_code) { error = error_code; } );

	so_5::close_retain_content( ch );

	child1.join();
	child2.join();

	UT_CHECK_CONDITION( error && so_5::rc_prepared_select_is_active_now == *error );
}

UT_UNIT_TEST( test_parallel_select )
{
	auto params = build_mchain_params();
	for( const auto & p : params )
	{
		cout << "=== " << p.first << " ===" << endl;

		run_with_time_limit(
			[&p]()
			{
				so_5::wrapped_env_t env;

				do_check_parallel_select(
						env.environment().create_mchain( p.second ),
						env.environment().create_mchain( p.second ) );
			},
			20,
			"test_parallel_select: " + p.first );
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
	UT_RUN_UNIT_TEST( test_parallel_select )

	return 0;
}


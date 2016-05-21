/*
 * Test for size-limited mchains without abort_app overflow reaction.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

#include <utest_helper_1/h/helper.hpp>

using namespace std;
using namespace chrono;

namespace props = so_5::mchain_props;

void
do_check_no_wait_drop_newest_impl(
	so_5::environment_t & env,
	const char * case_name,
	props::memory_usage_t memory )
{
	cout << "no_wait, drop_newest, " << case_name << ": " << std::flush;

	auto ch = env.create_mchain(
			so_5::make_limited_without_waiting_mchain_params(
					3,
					memory,
					props::overflow_reaction_t::drop_newest ) );

	so_5::send< int >( ch, 1 );
	so_5::send< int >( ch, 2 );
	so_5::send< int >( ch, 3 );
	so_5::send< int >( ch, 4 );

	receive( from(ch).handle_n(4).empty_timeout(so_5::no_wait),
			[]( int i ) { UT_CHECK_CONDITION( i < 4 ); } );

	cout << "OK" << std::endl;
}

void
do_check_no_wait_drop_newest( so_5::environment_t & env )
{
	do_check_no_wait_drop_newest_impl(
			env, "dynamic", props::memory_usage_t::dynamic );
	do_check_no_wait_drop_newest_impl(
			env, "prealloc", props::memory_usage_t::preallocated );
}

void
do_check_no_wait_remove_oldest_impl(
	so_5::environment_t & env,
	const char * case_name,
	props::memory_usage_t memory )
{
	cout << "no_wait, remove_oldest, " << case_name << ": " << std::flush;

	auto ch = env.create_mchain(
			so_5::make_limited_without_waiting_mchain_params(
					3,
					memory,
					props::overflow_reaction_t::remove_oldest ) );

	so_5::send< int >( ch, 1 );
	so_5::send< int >( ch, 2 );
	so_5::send< int >( ch, 3 );
	so_5::send< int >( ch, 4 );

	receive( from(ch).handle_n(4).empty_timeout(so_5::no_wait),
			[]( int i ) { UT_CHECK_CONDITION( i > 1 ); } );

	cout << "OK" << std::endl;
}

void
do_check_no_wait_remove_oldest( so_5::environment_t & env )
{
	do_check_no_wait_remove_oldest_impl(
			env, "dynamic", props::memory_usage_t::dynamic );
	do_check_no_wait_remove_oldest_impl(
			env, "prealloc", props::memory_usage_t::preallocated );
}

void
do_check_no_wait_throw_exception_impl(
	so_5::environment_t & env,
	const char * case_name,
	props::memory_usage_t memory )
{
	cout << "no_wait, throw_exception, " << case_name << ": " << std::flush;

	auto ch = env.create_mchain(
			so_5::make_limited_without_waiting_mchain_params(
					3,
					memory,
					props::overflow_reaction_t::throw_exception ) );

	so_5::send< int >( ch, 1 );
	so_5::send< int >( ch, 2 );
	so_5::send< int >( ch, 3 );
	try
	{
		so_5::send< int >( ch, 4 );
		ensure_or_die( false, "An exception must be throw before this line!" );
	}
	catch( const so_5::exception_t & ex )
	{
		UT_CHECK_CONDITION( so_5::rc_msg_chain_overflow == ex.error_code() );
	}

	cout << "OK" << std::endl;
}

void
do_check_no_wait_throw_exception( so_5::environment_t & env )
{
	do_check_no_wait_throw_exception_impl(
			env, "dynamic", props::memory_usage_t::dynamic );
	do_check_no_wait_throw_exception_impl(
			env, "prealloc", props::memory_usage_t::preallocated );
}

void
do_test_no_wait( bool use_tracer )
{
	const std::string env_desc = use_tracer ? "msg_tracing" : "no_msg_tracing";
	cout << "test_no_wait + " << env_desc << ": " << std::endl;

	run_with_time_limit(
		[use_tracer] {
			so_5::environment_params_t params;
			if( use_tracer )
				params.message_delivery_tracer(
						so_5::msg_tracing::std_clog_tracer() );

			so_5::wrapped_env_t env{ std::move(params) };

			do_check_no_wait_drop_newest( env.environment() );
			do_check_no_wait_remove_oldest( env.environment() );
			do_check_no_wait_throw_exception( env.environment() );
		},
		20,
		"test_no_wait + " + env_desc );
}
UT_UNIT_TEST( test_no_wait )
{
	do_test_no_wait( false );
	do_test_no_wait( true );
}

const chrono::milliseconds wait_timeout{ 100 };
const chrono::milliseconds timeout_delta{ 20 };

template< typename L >
void check_pause( L lambda )
{
	auto s = chrono::steady_clock::now();
	lambda();
	auto f = chrono::steady_clock::now();

	UT_CHECK_CONDITION( s + (wait_timeout - timeout_delta) < f );
}

void
do_check_wait_drop_newest_impl(
	so_5::environment_t & env,
	const char * case_name,
	props::memory_usage_t memory )
{
	cout << "wait, drop_newest, " << case_name << ": " << std::flush;

	auto ch = env.create_mchain(
			so_5::make_limited_with_waiting_mchain_params(
					3,
					memory,
					props::overflow_reaction_t::drop_newest,
					wait_timeout ) );

	so_5::send< int >( ch, 1 );
	so_5::send< int >( ch, 2 );
	so_5::send< int >( ch, 3 );
	check_pause( [&] { so_5::send< int >( ch, 4 ); } );

	receive( from(ch).handle_n(4).empty_timeout(so_5::no_wait),
			[]( int i ) { UT_CHECK_CONDITION( i < 4 ); } );

	cout << "OK" << std::endl;
}

void
do_check_wait_drop_newest( so_5::environment_t & env )
{
	do_check_wait_drop_newest_impl(
			env, "dynamic", props::memory_usage_t::dynamic );
	do_check_wait_drop_newest_impl(
			env, "prealloc", props::memory_usage_t::preallocated );
}

void
do_check_wait_remove_oldest_impl(
	so_5::environment_t & env,
	const char * case_name,
	props::memory_usage_t memory )
{
	cout << "wait, remove_oldest, " << case_name << ": " << std::flush;

	auto ch = env.create_mchain(
			so_5::make_limited_with_waiting_mchain_params(
					3,
					memory,
					props::overflow_reaction_t::remove_oldest,
					wait_timeout ) );

	so_5::send< int >( ch, 1 );
	so_5::send< int >( ch, 2 );
	so_5::send< int >( ch, 3 );
	check_pause( [&] { so_5::send< int >( ch, 4 ); } );

	receive( from(ch).handle_n(4).empty_timeout(so_5::no_wait),
			[]( int i ) { UT_CHECK_CONDITION( i > 1 ); } );

	cout << "OK" << std::endl;
}

void
do_check_wait_remove_oldest( so_5::environment_t & env )
{
	do_check_wait_remove_oldest_impl(
			env, "dynamic", props::memory_usage_t::dynamic );
	do_check_wait_remove_oldest_impl(
			env, "prealloc", props::memory_usage_t::preallocated );
}

void
do_check_wait_throw_exception_impl(
	so_5::environment_t & env,
	const char * case_name,
	props::memory_usage_t memory )
{
	cout << "wait, throw_exception, " << case_name << ": " << std::flush;

	auto ch = env.create_mchain(
			so_5::make_limited_with_waiting_mchain_params(
					3,
					memory,
					props::overflow_reaction_t::throw_exception,
					wait_timeout ) );

	so_5::send< int >( ch, 1 );
	so_5::send< int >( ch, 2 );
	so_5::send< int >( ch, 3 );
	check_pause( [&] {
		try
		{
			so_5::send< int >( ch, 4 );
			ensure_or_die( false, "An exception must be throw before this line!" );
		}
		catch( const so_5::exception_t & ex )
		{
			UT_CHECK_CONDITION( so_5::rc_msg_chain_overflow == ex.error_code() );
		}
	} );

	cout << "OK" << std::endl;
}

void
do_check_wait_throw_exception( so_5::environment_t & env )
{
	do_check_wait_throw_exception_impl(
			env, "dynamic", props::memory_usage_t::dynamic );
	do_check_wait_throw_exception_impl(
			env, "prealloc", props::memory_usage_t::preallocated );
}

void
do_test_wait( bool use_tracer )
{
	const std::string env_desc = use_tracer ? "msg_tracing" : "no_msg_tracing";
	cout << "test_wait + " << env_desc << ": " << std::endl;

	run_with_time_limit(
		[use_tracer] {
			so_5::environment_params_t params;
			if( use_tracer )
				params.message_delivery_tracer(
						so_5::msg_tracing::std_clog_tracer() );

			so_5::wrapped_env_t env{ std::move(params) };

			do_check_wait_drop_newest( env.environment() );
			do_check_wait_remove_oldest( env.environment() );
			do_check_wait_throw_exception( env.environment() );
		},
		20,
		"test_wait + " + env_desc );
}

UT_UNIT_TEST( test_wait )
{
	do_test_wait( false );
	do_test_wait( true );
}

int
main()
{
	UT_RUN_UNIT_TEST( test_no_wait )
	UT_RUN_UNIT_TEST( test_wait )

	return 0;
}


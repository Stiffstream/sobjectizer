/*
 * A test for using functions as handlers.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

#include <utest_helper_1/h/helper.hpp>

#include "../mchain_params.hpp"

using namespace std;

struct bye {};
bool g_bye_received = false;

void on_bye_1(bye) {
	g_bye_received = true;
}

void on_bye_2(so_5::mhood_t<bye>) {
	g_bye_received = true;
}

void on_bye_3(so_5::mutable_mhood_t<bye>) {
	g_bye_received = true;
}

void
check_by_value(so_5::mchain_t chain )
{
	so_5::send< bye >( chain );

	g_bye_received = false;
	auto r = receive(
			chain,
			so_5::no_wait,
			&on_bye_1 );

	UT_CHECK_CONDITION( 1 == r.extracted() );
	UT_CHECK_CONDITION( 1 == r.handled() );
	UT_CHECK_CONDITION( g_bye_received );
}

void
check_ref_by_value(so_5::mchain_t chain )
{
	so_5::send< bye >( chain );

	g_bye_received = false;
	auto r = receive(
			chain,
			so_5::no_wait,
			on_bye_1 );

	UT_CHECK_CONDITION( 1 == r.extracted() );
	UT_CHECK_CONDITION( 1 == r.handled() );
	UT_CHECK_CONDITION( g_bye_received );
}

void
check_by_mhood(so_5::mchain_t chain )
{
	so_5::send< bye >( chain );

	g_bye_received = false;
	auto r = receive(
			chain,
			so_5::no_wait,
			&on_bye_2 );

	UT_CHECK_CONDITION( 1 == r.extracted() );
	UT_CHECK_CONDITION( 1 == r.handled() );
	UT_CHECK_CONDITION( g_bye_received );
}

void
check_ref_by_mhood(so_5::mchain_t chain )
{
	so_5::send< bye >( chain );

	g_bye_received = false;
	auto r = receive(
			chain,
			so_5::no_wait,
			on_bye_2 );

	UT_CHECK_CONDITION( 1 == r.extracted() );
	UT_CHECK_CONDITION( 1 == r.handled() );
	UT_CHECK_CONDITION( g_bye_received );
}

void
check_by_mutable_mhood(so_5::mchain_t chain )
{
	so_5::send< so_5::mutable_msg<bye> >( chain );

	g_bye_received = false;
	auto r = receive(
			chain,
			so_5::no_wait,
			&on_bye_3 );

	UT_CHECK_CONDITION( 1 == r.extracted() );
	UT_CHECK_CONDITION( 1 == r.handled() );
	UT_CHECK_CONDITION( g_bye_received );
}

void
check_ref_by_mutable_mhood(so_5::mchain_t chain )
{
	so_5::send< so_5::mutable_msg<bye> >( chain );

	g_bye_received = false;
	auto r = receive(
			chain,
			so_5::no_wait,
			on_bye_3 );

	UT_CHECK_CONDITION( 1 == r.extracted() );
	UT_CHECK_CONDITION( 1 == r.handled() );
	UT_CHECK_CONDITION( g_bye_received );
}

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				struct hello : public so_5::signal_t {};

				so_5::wrapped_env_t env;

				auto params = build_mchain_params();

				for( const auto & p : params )
				{
					cout << "=== " << p.first << " ===" << endl;

					auto chain = env.environment().create_mchain( p.second );

					check_by_value( chain );
					check_ref_by_value( chain );

					check_by_mhood( chain );
					check_ref_by_mhood( chain );

					check_by_mutable_mhood( chain );
					check_ref_by_mutable_mhood( chain );
				}
			},
			20 );
	}
	catch( const exception & ex )
	{
		cerr << "Error: " << ex.what() << endl;
		return 1;
	}

	return 0;
}


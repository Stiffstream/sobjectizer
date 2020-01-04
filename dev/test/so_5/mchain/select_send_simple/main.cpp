/*
 * A simple test for simple select with send_case.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

#include <test/3rd_party/utest_helper/helper.hpp>

using namespace std::chrono_literals;

UT_UNIT_TEST( simple_failed_send_attempt )
{
	run_with_time_limit(
		[]()
		{
			struct hello {};

			so_5::wrapped_env_t env;

			auto ch = so_5::create_mchain( env.environment(),
					2,
					so_5::mchain_props::memory_usage_t::preallocated,
					so_5::mchain_props::overflow_reaction_t::abort_app );

			so_5::send< hello >( ch );
			so_5::send< hello >( ch );

			bool send_successed = false;

			auto r = so_5::select(
					so_5::from_all().handle_n(1).no_wait_on_empty(),
					send_case( ch, so_5::message_holder_t< hello >::make(),
							[&send_successed] {
								send_successed = true;
							} )
				);

			UT_CHECK_CONDITION( !r.was_sent() );
			UT_CHECK_CONDITION( !send_successed );
		},
		20 );
}

UT_UNIT_TEST( simple_failed_send_attempt_2 )
{
	run_with_time_limit(
		[]()
		{
			struct hello {};

			so_5::wrapped_env_t env;

			auto ch = so_5::create_mchain( env.environment(),
					2,
					so_5::mchain_props::memory_usage_t::preallocated,
					so_5::mchain_props::overflow_reaction_t::abort_app );

			so_5::send< hello >( ch );
			so_5::send< hello >( ch );

			bool send_successed = false;

			const auto timeout = 250ms;
			const auto started_at = std::chrono::steady_clock::now();
			auto r = so_5::select(
					so_5::from_all().handle_n(1).total_time(timeout),
					send_case( ch, so_5::message_holder_t< hello >::make(),
							[&send_successed] {
								send_successed = true;
							} )
				);
			const auto finished_at = std::chrono::steady_clock::now();

			UT_CHECK_CONDITION( !r.was_sent() );
			UT_CHECK_CONDITION( !send_successed );
			UT_CHECK_CONDITION( timeout + 10ms /* some delta */ >
					finished_at - started_at );
		},
		20 );
}

UT_UNIT_TEST( simple_success_send_attempt )
{
	run_with_time_limit(
		[]()
		{
			struct hello {};

			so_5::wrapped_env_t env;

			auto ch = so_5::create_mchain( env.environment(),
					2,
					so_5::mchain_props::memory_usage_t::preallocated,
					so_5::mchain_props::overflow_reaction_t::abort_app );

			so_5::send< hello >( ch );

			bool send_successed = false;

			auto r = so_5::select(
					so_5::from_all().handle_n(1).no_wait_on_empty(),
					send_case( ch, so_5::message_holder_t< hello >::make(),
							[&send_successed] {
								send_successed = true;
							} )
				);

				UT_CHECK_CONDITION( r.was_sent() );
				UT_CHECK_CONDITION( send_successed );
				UT_CHECK_CONDITION( 2u == ch->size() );
		},
		20 );
}

UT_UNIT_TEST( send_with_receive )
{
	run_with_time_limit(
		[]()
		{
			struct hello {};

			so_5::wrapped_env_t env;

			auto ch1 = so_5::create_mchain( env.environment(),
					2,
					so_5::mchain_props::memory_usage_t::preallocated,
					so_5::mchain_props::overflow_reaction_t::abort_app );

			so_5::send< hello >( ch1 );
			so_5::send< hello >( ch1 );

			auto ch2 = so_5::create_mchain( env.environment() );
			so_5::send_delayed< hello >( ch2, std::chrono::milliseconds{250} );

			bool send_successed = false;

			auto r = so_5::select(
					so_5::from_all().handle_all(),
					send_case( ch1, so_5::message_holder_t< hello >::make(),
							[&send_successed] {
								send_successed = true;
							} ),
					receive_case( ch2, [&ch1](hello) {
								// Make free space in ch1.
								so_5::receive(
										so_5::from(ch1).handle_n(1).no_wait_on_empty(),
										[](hello) {} );
							} )
				);

				UT_CHECK_CONDITION( r.was_handled() );
				UT_CHECK_CONDITION( r.was_sent() );
				UT_CHECK_CONDITION( send_successed );
				UT_CHECK_CONDITION( 2u == ch1->size() );
		},
		20 );
}

UT_UNIT_TEST( two_sends_with_receive )
{
	run_with_time_limit(
		[]()
		{
			struct hello {};

			so_5::wrapped_env_t env;

			auto ch1 = so_5::create_mchain( env.environment(),
					2,
					so_5::mchain_props::memory_usage_t::preallocated,
					so_5::mchain_props::overflow_reaction_t::abort_app );

			so_5::send< hello >( ch1 );
			so_5::send< hello >( ch1 );

			auto ch2 = so_5::create_mchain( env.environment(),
					2,
					so_5::mchain_props::memory_usage_t::preallocated,
					so_5::mchain_props::overflow_reaction_t::abort_app );

			so_5::send< hello >( ch2 );
			so_5::send< hello >( ch2 );

			auto ch3 = so_5::create_mchain( env.environment() );
			so_5::send_delayed< hello >( ch3, std::chrono::milliseconds{250} );

			bool send_successed = false;

			auto r = so_5::select(
					so_5::from_all().handle_all(),
					send_case( ch1, so_5::message_holder_t< hello >::make(),
							[&send_successed] {
								send_successed = true;
							} ),
					send_case( ch2, so_5::message_holder_t< hello >::make(), [](){} ),
					receive_case( ch3, [&ch1](hello) {
								// Make free space in ch1.
								so_5::receive(
										so_5::from(ch1).handle_n(1).no_wait_on_empty(),
										[](hello) {} );
							} )
				);

				UT_CHECK_CONDITION( r.was_handled() );
				UT_CHECK_CONDITION( r.was_sent() );
				UT_CHECK_CONDITION( send_successed );
				UT_CHECK_CONDITION( 2u == ch1->size() );
				UT_CHECK_CONDITION( 2u == ch2->size() );
		},
		20 );
}

UT_UNIT_TEST( three_sends )
{
	run_with_time_limit(
		[]()
		{
			struct hello {};

			so_5::wrapped_env_t env;

			auto ch1 = so_5::create_mchain( env.environment(),
					2,
					so_5::mchain_props::memory_usage_t::preallocated,
					so_5::mchain_props::overflow_reaction_t::abort_app );

			so_5::send< hello >( ch1 );
			so_5::send< hello >( ch1 );

			auto ch2 = so_5::create_mchain( env.environment(),
					2,
					so_5::mchain_props::memory_usage_t::preallocated,
					so_5::mchain_props::overflow_reaction_t::abort_app );

			so_5::send< hello >( ch2 );
			so_5::send< hello >( ch2 );

			auto ch3 = so_5::create_mchain( env.environment(),
					2,
					so_5::mchain_props::memory_usage_t::preallocated,
					so_5::mchain_props::overflow_reaction_t::abort_app );

			so_5::send< hello >( ch3 );

			int used_chain = 0;

			auto r = so_5::select(
					so_5::from_all().handle_all(),
					send_case( ch1, so_5::message_holder_t< hello >::make(),
							[&used_chain] {
								used_chain = 1;
							} ),
					send_case( ch2, so_5::message_holder_t< hello >::make(),
							[&used_chain] {
								used_chain = 2;
							} ),
					send_case( ch3, so_5::message_holder_t< hello >::make(),
							[&used_chain] {
								used_chain = 3;
							} )
				);

				UT_CHECK_CONDITION( !r.was_handled() );
				UT_CHECK_CONDITION( r.was_sent() );
				UT_CHECK_CONDITION( 3 == used_chain );
				UT_CHECK_CONDITION( 2u == ch1->size() );
				UT_CHECK_CONDITION( 2u == ch2->size() );
				UT_CHECK_CONDITION( 2u == ch3->size() );
		},
		20 );
}

UT_UNIT_TEST( send_when_closed_drop_content )
{
	run_with_time_limit(
		[]()
		{
			struct hello {};

			so_5::wrapped_env_t env;

			auto ch1 = so_5::create_mchain( env.environment(),
					2,
					so_5::mchain_props::memory_usage_t::preallocated,
					so_5::mchain_props::overflow_reaction_t::abort_app );

			so_5::send< hello >( ch1 );
			so_5::send< hello >( ch1 );

			auto ch2 = so_5::create_mchain( env.environment() );
			so_5::send_delayed< hello >( ch2, std::chrono::milliseconds{250} );

			bool send_successed = false;

			auto r = so_5::select(
					so_5::from_all().handle_all(),
					send_case( ch1, so_5::message_holder_t< hello >::make(),
							[&send_successed] {
								send_successed = true;
							} ),
					receive_case( ch2, [&ch1](hello) {
								so_5::close_drop_content( ch1 );
							} )
				);

				UT_CHECK_CONDITION( r.was_handled() );
				UT_CHECK_CONDITION( !r.was_sent() );
				UT_CHECK_CONDITION( !send_successed );
		},
		20 );
}

UT_UNIT_TEST( send_when_closed_retain_content )
{
	run_with_time_limit(
		[]()
		{
			struct hello {};

			so_5::wrapped_env_t env;

			auto ch1 = so_5::create_mchain( env.environment(),
					2,
					so_5::mchain_props::memory_usage_t::preallocated,
					so_5::mchain_props::overflow_reaction_t::abort_app );

			so_5::send< hello >( ch1 );
			so_5::send< hello >( ch1 );

			auto ch2 = so_5::create_mchain( env.environment() );
			so_5::send_delayed< hello >( ch2, std::chrono::milliseconds{250} );

			bool send_successed = false;

			auto r = so_5::select(
					so_5::from_all().handle_all(),
					send_case( ch1, so_5::message_holder_t< hello >::make(),
							[&send_successed] {
								send_successed = true;
							} ),
					receive_case( ch2, [&ch1](hello) {
								so_5::close_retain_content( ch1 );
							} )
				);

				UT_CHECK_CONDITION( r.was_handled() );
				UT_CHECK_CONDITION( !r.was_sent() );
				UT_CHECK_CONDITION( !send_successed );
		},
		20 );
}

int
main()
{
	UT_RUN_UNIT_TEST( simple_failed_send_attempt )
	UT_RUN_UNIT_TEST( simple_failed_send_attempt_2 )
	UT_RUN_UNIT_TEST( simple_success_send_attempt )
	UT_RUN_UNIT_TEST( send_with_receive )
	UT_RUN_UNIT_TEST( two_sends_with_receive )
	UT_RUN_UNIT_TEST( three_sends )
	UT_RUN_UNIT_TEST( send_when_closed_drop_content )
	UT_RUN_UNIT_TEST( send_when_closed_retain_content )

	return 0;
}


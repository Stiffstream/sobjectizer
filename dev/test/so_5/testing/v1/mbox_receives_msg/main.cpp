#include <so_5/experimental/testing/v1/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

#include <test/3rd_party/utest_helper/helper.hpp>

using namespace std::chrono_literals;

namespace so5_tests = so_5::experimental::testing::v1;

namespace test
{

struct test_signal final : public so_5::signal_t {};

struct test_msg final : public so_5::message_t {};

struct test_msg_with_content final : public so_5::message_t
	{
		int m_value;

		test_msg_with_content( int v ) : m_value{ v }
			{}
	};

template< typename Msg >
class a_initiator_t final : public so_5::agent_t
{
	const so_5::mbox_t m_dest;

public:
	a_initiator_t( context_t ctx, so_5::mbox_t dest )
		: so_5::agent_t{ std::move(ctx) }
		, m_dest{ std::move(dest) }
	{}

	void
	so_evt_start() override
	{
		so_5::send< Msg >( m_dest );
	}
};

} /* namespace test */

using namespace test;

UT_UNIT_TEST( signal )
{
	run_with_time_limit(
		[]()
		{
			so5_tests::testing_env_t env;

			so_5::mbox_t dest = env.environment().create_mbox();
			env.environment().introduce_coop(
				[dest](so_5::coop_t & coop) {
					return coop.make_agent< a_initiator_t< test_signal > >( dest );
				} );

			env.scenario().define_step( "single" )
				.when( dest
						& so5_tests::receives< test_signal >() )
				;

			env.scenario().run_for( 1000ms );

			UT_CHECK_EQ( so5_tests::completed(), env.scenario().result() );
		},
		5 );
}

UT_UNIT_TEST( immutable_msg )
{
	run_with_time_limit(
		[]()
		{
			so5_tests::testing_env_t env;

			so_5::mbox_t dest = env.environment().create_mbox();
			env.environment().introduce_coop(
				[dest](so_5::coop_t & coop) {
					return coop.make_agent< a_initiator_t< test_msg > >( dest );
				} );

			env.scenario().define_step( "single" )
				.when( dest
						& so5_tests::receives< test_msg >() )
				;

			env.scenario().run_for( 1000ms );

			UT_CHECK_EQ( so5_tests::completed(), env.scenario().result() );
		},
		5 );
}

UT_UNIT_TEST( mutable_msg )
{
	run_with_time_limit(
		[]()
		{
			so5_tests::testing_env_t env;

			so_5::mbox_t dest = so_5::make_unique_subscribers_mbox( env.environment() );
			env.environment().introduce_coop(
				[dest](so_5::coop_t & coop) {
					return coop.make_agent< a_initiator_t< so_5::mutable_msg< test_msg > > >( dest );
				} );

			env.scenario().define_step( "single" )
				.when( dest
						& so5_tests::receives< so_5::mutable_msg< test_msg > >() )
				;

			env.scenario().run_for( 1000ms );

			UT_CHECK_EQ( so5_tests::completed(), env.scenario().result() );
		},
		5 );
}

UT_UNIT_TEST( receives_with_inspect_immutable )
{
	run_with_time_limit(
		[]()
		{
			so5_tests::testing_env_t env;

			so_5::mbox_t dest = so_5::make_unique_subscribers_mbox( env.environment() );

			env.scenario().define_step( "single" )
				.impact< test_msg_with_content >( dest, 42 )
				.when( dest
						& so5_tests::receives< test_msg_with_content >()
						& so5_tests::inspect_msg( "inspection",
							[]( const test_msg_with_content & msg ) -> std::string {
								return 42 == msg.m_value ? "OK" : "FAIL";
							} ) )
				;

			env.scenario().run_for( 1000ms );

			UT_CHECK_EQ( so5_tests::completed(), env.scenario().result() );
			UT_CHECK_EQ( "OK", env.scenario().stored_msg_inspection_result(
						"single", "inspection" ) );
		},
		5 );
}

UT_UNIT_TEST( receives_with_inspect_mutable )
{
	run_with_time_limit(
		[]()
		{
			so5_tests::testing_env_t env;

			so_5::mbox_t dest = so_5::make_unique_subscribers_mbox( env.environment() );

			env.scenario().define_step( "single" )
				.impact< so_5::mutable_msg<test_msg_with_content> >( dest, 42 )
				.when( dest
						& so5_tests::receives< so_5::mutable_msg<test_msg_with_content> >()
						& so5_tests::inspect_msg(
							"inspection",
							[]( const test_msg_with_content & msg ) -> std::string {
								return 42 == msg.m_value ? "OK" : "FAIL";
							} ) )
				;

			env.scenario().run_for( 1000ms );

			UT_CHECK_EQ( so5_tests::completed(), env.scenario().result() );
			UT_CHECK_EQ( "OK", env.scenario().stored_msg_inspection_result(
						"single", "inspection" ) );
		},
		5 );
}

UT_UNIT_TEST( several_receives_with_inspect_immutable )
{
	run_with_time_limit(
		[]()
		{
			so5_tests::testing_env_t env;

			so_5::mbox_t dest = env.environment().create_mbox();

			env.scenario().define_step( "one" )
				.impact< test_msg_with_content >( dest, 1 )
				.when( dest
						& so5_tests::receives< test_msg_with_content >()
						& so5_tests::inspect_msg( "inspection",
							[]( const test_msg_with_content & msg ) -> std::string {
								return 1 == msg.m_value ? "OK" : "FAIL";
							} ) )
				;
			env.scenario().define_step( "two" )
				.impact< test_msg_with_content >( dest, 2 )
				.when( dest
						& so5_tests::receives< test_msg_with_content >()
						& so5_tests::inspect_msg( "inspection",
							[]( const test_msg_with_content & msg ) -> std::string {
								return 2 == msg.m_value ? "OK" : "FAIL";
							} ) )
				;
			env.scenario().define_step( "three" )
				.impact< test_msg_with_content >( dest, 3 )
				.when( dest
						& so5_tests::receives< test_msg_with_content >()
						& so5_tests::inspect_msg( "inspection",
							[]( const test_msg_with_content & msg ) -> std::string {
								return 3 == msg.m_value ? "OK" : "FAIL";
							} ) )
				;

			env.scenario().run_for( 1000ms );

			UT_CHECK_EQ( so5_tests::completed(), env.scenario().result() );
			UT_CHECK_EQ( "OK", env.scenario().stored_msg_inspection_result(
						"one", "inspection" ) );
			UT_CHECK_EQ( "OK", env.scenario().stored_msg_inspection_result(
						"two", "inspection" ) );
			UT_CHECK_EQ( "OK", env.scenario().stored_msg_inspection_result(
						"three", "inspection" ) );
		},
		5 );
}
int
main()
{
	UT_RUN_UNIT_TEST( signal )
	UT_RUN_UNIT_TEST( immutable_msg )
	UT_RUN_UNIT_TEST( mutable_msg )
	UT_RUN_UNIT_TEST( receives_with_inspect_immutable )
	UT_RUN_UNIT_TEST( receives_with_inspect_mutable )
	UT_RUN_UNIT_TEST( several_receives_with_inspect_immutable )

	return 0;
}


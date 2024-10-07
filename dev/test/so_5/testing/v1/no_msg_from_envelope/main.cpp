#include <so_5/experimental/testing/v1/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

#include <test/3rd_party/utest_helper/helper.hpp>

namespace so5_tests = so_5::experimental::testing::v1;

namespace test
{

class test_envelope_t : public so_5::enveloped_msg::envelope_t
{
	std::atomic<int> & m_how_many_times_handled;
	std::atomic<int> & m_how_many_times_inspected;

public:
	test_envelope_t(
		std::atomic<int> & how_many_times_handled,
		std::atomic<int> & how_many_times_inspected )
		: m_how_many_times_handled{ how_many_times_handled }
		, m_how_many_times_inspected{ how_many_times_inspected }
	{}

	void
	access_hook(
		access_context_t context,
		handler_invoker_t & /*invoker*/ ) noexcept override
	{
		switch( context )
		{
		case access_context_t::handler_found:
			++m_how_many_times_handled;
		break;

		case access_context_t::transformation:
			// Nothing to do.
		break;

		case access_context_t::inspection:
			++m_how_many_times_inspected;
		break;
		}

		// Do not call invoker.invoke!
	}
};

struct hello final : public so_5::signal_t {};

class a_receiver_t final : public so_5::agent_t
{
public:
	using so_5::agent_t::agent_t;

	void
	so_define_agent() override
	{
		so_subscribe_self().event(
			[](mhood_t<hello>) {
				// This should not happen!
				std::abort();
			} );
	}
};

class a_noop_t final : public so_5::agent_t
{
	state_t st_unused{ this, "unused" };

public:
	using so_5::agent_t::agent_t;

	void
	so_define_agent() override
	{
		so_subscribe_self().in( st_unused )
			.event( [](mhood_t<hello>) {
					// This should not happen!
					std::abort();
				} );
	}
};

class a_sender_t final : public so_5::agent_t
{
	std::atomic<int> & m_how_many_times_handled;
	std::atomic<int> & m_how_many_times_inspected;
	const so_5::mbox_t m_target_mbox;

public:
	a_sender_t(
		context_t ctx,
		std::atomic<int> & how_many_times_handled,
		std::atomic<int> & how_many_times_inspected,
		so_5::mbox_t target_mbox )
		: so_5::agent_t{ std::move(ctx) }
		, m_how_many_times_handled{ how_many_times_handled }
		, m_how_many_times_inspected{ how_many_times_inspected }
		, m_target_mbox{ std::move(target_mbox) }
	{}

	void
	so_evt_start() override
	{
		m_target_mbox->do_deliver_message(
				so_5::message_delivery_mode_t::ordinary,
				so_5::message_payload_type< hello >::subscription_type_index(),
				std::make_unique< test_envelope_t >(
						m_how_many_times_handled,
						m_how_many_times_inspected ),
				1u );
	}
};

} /* namespace test */

using namespace test;

UT_UNIT_TEST( reacts_to )
{
	std::atomic<int> how_many_times_handled{ 0 };
	std::atomic<int> how_many_times_inspected{ 0 };
	run_with_time_limit(
		[&]()
		{
			so5_tests::testing_env_t env;

			a_receiver_t * receiver = env.environment().introduce_coop(
					[&how_many_times_handled, &how_many_times_inspected]
					(so_5::coop_t & coop) {
						a_receiver_t * r = coop.make_agent< a_receiver_t >();
						coop.make_agent< a_sender_t >(
								how_many_times_handled,
								how_many_times_inspected,
								r->so_direct_mbox() );
						return r;
					} );

			env.scenario().define_step( "hello-at-start" )
				.when( *receiver & so5_tests::reacts_to< hello >() )
				;

			env.scenario().run_for( std::chrono::milliseconds(200) );

			UT_CHECK_NE( so5_tests::completed(), env.scenario().result() );
		},
		5 );

	UT_CHECK_EQ( 1, how_many_times_handled.load() );
	UT_CHECK_EQ( 0, how_many_times_inspected.load() );
}

UT_UNIT_TEST( ignores )
{
	std::atomic<int> how_many_times_handled{ 0 };
	std::atomic<int> how_many_times_inspected{ 0 };
	run_with_time_limit(
		[&]()
		{
			so5_tests::testing_env_t env;

			a_noop_t * receiver = env.environment().introduce_coop(
					[&how_many_times_handled, &how_many_times_inspected]
					(so_5::coop_t & coop) {
						auto * r = coop.make_agent< a_noop_t >();
						coop.make_agent< a_sender_t >(
								how_many_times_handled,
								how_many_times_inspected,
								r->so_direct_mbox() );
						return r;
					} );

			env.scenario().define_step( "hello-at-start" )
				.when( *receiver & so5_tests::ignores< hello >() )
				;

			env.scenario().run_for( std::chrono::milliseconds(200) );

			UT_CHECK_NE( so5_tests::completed(), env.scenario().result() );
		},
		5 );

	UT_CHECK_EQ( 0, how_many_times_handled.load() );
	UT_CHECK_EQ( 1, how_many_times_inspected.load() );
}

int
main()
{
	UT_RUN_UNIT_TEST( reacts_to )
	UT_RUN_UNIT_TEST( ignores )

	return 0;
}


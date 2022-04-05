#include <so_5/experimental/testing/v1/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

#include <test/3rd_party/utest_helper/helper.hpp>

namespace tests = so_5::experimental::testing;

class worker_t final : public so_5::agent_t
{
	const state_t st_free{ this, "free" };
	const state_t st_busy{ this, "busy" };

public :
	struct acquire final : public so_5::signal_t {};
	struct release final : public so_5::signal_t {};

	worker_t( context_t ctx, const so_5::mbox_t & control_mbox )
		:	so_5::agent_t{ std::move(ctx) }
	{
		st_free.event(
				control_mbox,
				[this](mhood_t<acquire>) { st_busy.activate(); } );
		st_busy.event(
				control_mbox,
				[this](mhood_t<release>) { st_free.activate(); } );

		this >>= st_free;
	}
};

class manager_t final : public so_5::agent_t
{
	const so_5::mbox_t m_control_mbox;

	struct start_work final : public so_5::signal_t {};
	struct finish_work final : public so_5::signal_t {};

public :
	manager_t( context_t ctx, so_5::mbox_t control_mbox )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_control_mbox{ std::move(control_mbox) }
	{
		so_subscribe_self()
			.event( [this](mhood_t<start_work>) {
					so_5::send< worker_t::acquire >( m_control_mbox );
					so_5::send_delayed< finish_work >( *this,
							std::chrono::milliseconds(150) );
				} )
			.event( [this](mhood_t<finish_work>) {
					so_5::send< worker_t::release >( m_control_mbox );
				} );
	}

	void so_evt_start() override
	{
		so_5::send_delayed< start_work >( *this,
				std::chrono::milliseconds(75) );
	}
};

UT_UNIT_TEST( workers_and_manager )
{
	run_with_time_limit(
		[]()
		{
			tests::testing_env_t env{
				[]( so_5::environment_params_t & params ) {
					(void)params;
#if 0
					params.message_delivery_tracer(
						so_5::msg_tracing::std_clog_tracer() );
#endif
				} };

			const auto control_mbox = env.environment().create_mbox();

			auto [first_worker, second_worker] =
				env.environment().introduce_coop(
					[&](so_5::coop_t & coop) {
						coop.make_agent< manager_t >( control_mbox );

						return std::make_tuple(
								coop.make_agent< worker_t >( control_mbox ),
								coop.make_agent< worker_t >( control_mbox ) );
					} );

			auto scenario = env.scenario();

			scenario.define_step( "acquire" )
				.when_all(
						*first_worker
							& tests::reacts_to< worker_t::acquire >( control_mbox )
							& tests::store_state_name( "first" ),
						*second_worker
							& tests::reacts_to< worker_t::acquire >( control_mbox )
							& tests::store_state_name( "second" ) )
				.constraints(
						tests::not_before( std::chrono::milliseconds(50) ),
						tests::not_after( std::chrono::milliseconds(1000) ) ) ;

			scenario.define_step( "release" )
				.when_all(
						*first_worker
							& tests::reacts_to< worker_t::release >( control_mbox )
							& tests::store_state_name( "first" ),
						*second_worker
							& tests::reacts_to< worker_t::release >( control_mbox )
							& tests::store_state_name( "second" ) )
				.constraints(
						tests::not_before( std::chrono::milliseconds(75) ) );

			scenario.run_for( std::chrono::seconds(4) );

			UT_CHECK_EQ( tests::completed(), scenario.result() );

			UT_CHECK_EQ( "busy", scenario.stored_state_name("acquire", "first") );
			UT_CHECK_EQ( "busy", scenario.stored_state_name("acquire", "second") );

			UT_CHECK_EQ( "free", scenario.stored_state_name("release", "first") );
			UT_CHECK_EQ( "free", scenario.stored_state_name("release", "second") );
		},
		5 );
}

int
main()
{
	UT_RUN_UNIT_TEST( workers_and_manager )

	return 0;
}


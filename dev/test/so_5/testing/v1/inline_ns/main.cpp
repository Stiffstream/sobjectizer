#include <so_5/experimental/testing/v1/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

#include <utest_helper_1/h/helper.hpp>

#if defined(SO_5_NO_SUPPORT_FOR_INLINE_NAMESPACE)
namespace tests = so_5::experimental::testing::v1;
#else
namespace tests = so_5::experimental::testing;
#endif

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
				[this](mhood_t<acquire>) { this >>= st_busy; } );
		st_busy.event(
				control_mbox,
				[this](mhood_t<release>) { this >>= st_free; } );

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
							std::chrono::milliseconds(100) );
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
			tests::testing_env_t env;

			so_5::agent_t * first_worker{};
			so_5::agent_t * second_worker{};
			so_5::agent_t * manager{};

			const auto control_mbox = env.environment().create_mbox();

			env.environment().introduce_coop( [&](so_5::coop_t & coop) {
					first_worker = coop.make_agent< worker_t >( control_mbox );
					second_worker = coop.make_agent< worker_t >( control_mbox );

					manager = coop.make_agent< manager_t >( control_mbox );
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
						tests::not_after( std::chrono::milliseconds(150) ) );

			scenario.define_step( "release" )
				.when_all(
						*first_worker
							& tests::reacts_to< worker_t::release >( control_mbox )
							& tests::store_state_name( "first" ),
						*second_worker
							& tests::reacts_to< worker_t::release >( control_mbox )
							& tests::store_state_name( "second" ) )
				.constraints(
						tests::not_before( std::chrono::milliseconds(50) ) );

			scenario.run_for( std::chrono::seconds(1) );

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


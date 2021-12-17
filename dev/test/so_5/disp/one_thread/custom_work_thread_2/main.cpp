/*
 * Check for the usage of custom work thread factory.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

#include "../../custom_work_thread.hpp"

class a_test_t final : public so_5::agent_t
{
public:
	using so_5::agent_t::agent_t;

	void
	so_evt_start() override
	{
		so_deregister_agent_coop_normally();
	}
};

void
run_test()
{
	auto global_factory = std::make_shared< disp_tests::custom_work_thread_factory_t >();
	auto separate_factory = std::make_shared< disp_tests::custom_work_thread_factory_t >();

	so_5::launch( [&]( so_5::environment_t & env ) {
			env.introduce_coop( [&]( so_5::coop_t & coop ) {
					auto disp_params = so_5::disp::one_thread::disp_params_t{};
					disp_params.work_thread_factory( global_factory );

					auto disp = so_5::disp::one_thread::make_dispatcher(
							env,
							"demo_global_factory",
							disp_params );

					coop.make_agent_with_binder< a_test_t >( disp.binder() );
				} );

			env.introduce_coop( [&]( so_5::coop_t & coop ) {
					auto disp_params = so_5::disp::one_thread::disp_params_t{};
					disp_params.work_thread_factory( separate_factory );

					auto disp = so_5::disp::one_thread::make_dispatcher(
							env,
							"demo_separate_factory",
							disp_params );

					coop.make_agent_with_binder< a_test_t >( disp.binder() );
				} );
		},
		[global_factory]( so_5::environment_params_t & params ) {
			params.work_thread_factory( global_factory );
#if 0
			params.message_delivery_tracer(
					so_5::msg_tracing::std_cout_tracer() );
#endif
		} );

	// 2 because of default dispacther and demo_global_factory dispatcher.
	ensure_or_die( 2 == global_factory->started(),
			"unexpected number of started threads (global_factory)" );
	ensure_or_die( 2 == global_factory->finished(),
			"unexpected number of finished threads (global_factory)" );
	ensure_or_die( 2 == global_factory->created(),
			"unexpected number of created threads (global_factory)" );
	ensure_or_die( 2 == global_factory->destroyed(),
			"unexpected number of destroyed threads (global_factory)" );

	ensure_or_die( 1 == separate_factory->started(),
			"unexpected number of started threads (separate_factory)" );
	ensure_or_die( 1 == separate_factory->finished(),
			"unexpected number of finished threads (separate_factory)" );
	ensure_or_die( 1 == separate_factory->created(),
			"unexpected number of created threads (separate_factory)" );
	ensure_or_die( 1 == separate_factory->destroyed(),
			"unexpected number of destroyed threads (separate_factory)" );
}

int
main()
{
	try
	{
		run_with_time_limit(
			[]() {
				run_test();
			},
			5 );
	}
	catch(const std::exception & ex)
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


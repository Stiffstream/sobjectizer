/*
 * Check for the usage of custom work thread factory.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

#include "../../custom_work_thread.hpp"

namespace test
{

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
	auto factory = std::make_shared< disp_tests::custom_work_thread_factory_t >();

	so_5::launch( [&]( so_5::environment_t & env ) {
			auto disp_params = so_5::disp::nef_thread_pool::disp_params_t{};
			disp_params.thread_count( 3u );
			disp_params.work_thread_factory( factory );

			auto disp = so_5::disp::nef_thread_pool::make_dispatcher(
					env,
					"demo",
					disp_params );

			so_5::disp::nef_thread_pool::bind_params_t bind_params;
			for( int i = 0; i < 3; ++i )
			{
				env.introduce_coop( [&]( so_5::coop_t & coop ) {
						coop.make_agent_with_binder< a_test_t >(
								disp.binder( bind_params ) );
					} );
			}
		},
		[]( so_5::environment_params_t & /*params*/ ) {
#if 0
			params.message_delivery_tracer(
					so_5::msg_tracing::std_cout_tracer() );
#endif
		} );

	ensure_or_die( 3 == factory->started(),
			"unexpected number of started threads" );
	ensure_or_die( 3 == factory->finished(),
			"unexpected number of finished threads" );
	ensure_or_die( 3 == factory->created(),
			"unexpected number of created threads" );
	ensure_or_die( 3 == factory->destroyed(),
			"unexpected number of destroyed threads" );
}

} /* namespace test */

using namespace test;

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


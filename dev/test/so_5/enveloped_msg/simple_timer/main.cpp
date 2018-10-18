/*
 * Check for enveloped service request.
 */

#include <so_5/all.hpp>
#include <so_5/h/stdcpp.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

#include <utest_helper_1/h/helper.hpp>

#include "../common_stuff.hpp"

struct shutdown final : public so_5::signal_t {};

class test_case_t final : public so_5::agent_t
{
	so_5::outliving_reference_t< trace_t > m_trace;

public :
	test_case_t(
		context_t ctx,
		so_5::outliving_reference_t< trace_t > trace )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_trace{ trace }
	{
		so_subscribe_self()
			.event( [this, trace]( mhood_t<shutdown> ) {
					trace.get().append( "shutdown;" );
					so_deregister_agent_coop_normally();
				} );
	}

	void
	so_evt_start() override
	{
		so_5::message_ref_t envelope(
				so_5::stdcpp::make_unique< test_envelope_t >(
						m_trace,
						"mb",
						so_5::message_ref_t{} ) );

		so_environment().single_timer(
				so_5::message_payload_type< shutdown >::subscription_type_index(),
				std::move(envelope),
				so_direct_mbox(),
				std::chrono::milliseconds(50) );
	}
};

void
run_test()
{
	trace_t trace;
	so_5::launch( [&]( so_5::environment_t & env ) {
			env.introduce_coop( [&]( so_5::coop_t & coop ) {
					coop.make_agent< test_case_t >(
							so_5::outliving_mutable(trace) );
				} );
		},
		[]( so_5::environment_params_t & params ) {
			params.message_delivery_tracer(
					so_5::msg_tracing::std_cout_tracer() );
		} );

	std::cout << "trace is: " << trace.content() << std::endl;

	ensure_or_die(
			"mb:pre_invoke;shutdown;mb:post_invoke;"
					== trace.content(),
			"trace missmatch: " + trace.content() );
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


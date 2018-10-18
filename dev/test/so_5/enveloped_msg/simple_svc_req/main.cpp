/*
 * Check for enveloped service request.
 */

#include <so_5/all.hpp>
#include <so_5/h/stdcpp.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

#include <utest_helper_1/h/helper.hpp>

#include "../common_stuff.hpp"

struct so_based_request final : public so_5::message_t
{
	const std::string m_value;

	so_based_request( std::string value ) : m_value( std::move(value) ) {}
};

struct user_request final
{
	std::string m_value;
};

class service_t final : public so_5::agent_t
{
public :
	service_t(
		context_t ctx,
		const so_5::mbox_t & mbox,
		so_5::outliving_reference_t< trace_t > trace )
		:	so_5::agent_t{ std::move(ctx) }
	{
		so_subscribe( mbox )
			.event( [trace]( mhood_t<so_based_request> cmd ) -> std::string {
					trace.get().append( "so_based_request{" + cmd->m_value + "};" );
					return "<" + cmd->m_value + ">";
				} )
			.event( [trace]( mhood_t<user_request> cmd ) -> std::string {
					trace.get().append( "user_request{" + cmd->m_value + "};" );
					return "<" + cmd->m_value + ">";
				} );
	}
};

class customer_t final : public so_5::agent_t
{
	const so_5::mbox_t m_service;
	so_5::outliving_reference_t< trace_t > m_trace;

public :
	customer_t(
		context_t ctx,
		so_5::mbox_t mbox,
		so_5::outliving_reference_t< trace_t > trace )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_service{ std::move(mbox) }
		,	m_trace{ trace }
	{}

	void
	so_evt_start() override
	{
		m_trace.get().append( "issue-req-1;" );
		const auto r1 = so_5::request_value< std::string, so_based_request >(
				m_service, so_5::infinite_wait, "First" );
		// Do some time to append "post_invoke" on another thread.
		std::this_thread::sleep_for(std::chrono::milliseconds(25));
		m_trace.get().append( "req-1-result=" + r1 + ";" );

		m_trace.get().append( "issue-req-2;" );
		const auto r2 = so_5::request_value< std::string, user_request >(
				m_service, so_5::infinite_wait, "Second" );
		// Do some time to append "post_invoke" on another thread.
		std::this_thread::sleep_for(std::chrono::milliseconds(25));
		m_trace.get().append( "req-2-result=" + r2 + ";" );

		so_deregister_agent_coop_normally();
	}
};

void
run_test()
{
	trace_t trace;
	so_5::launch( [&]( so_5::environment_t & env ) {
			env.introduce_coop( [&]( so_5::coop_t & coop ) {
					auto mbox = special_mbox_t<>::make(
							env.create_mbox(),
							so_5::outliving_mutable(trace),
							"mb" );

					coop.make_agent_with_binder< service_t >(
							so_5::disp::one_thread::create_private_disp( env )->binder(),
							mbox,
							so_5::outliving_mutable(trace) );

					coop.make_agent< customer_t >(
							mbox,
							so_5::outliving_mutable(trace) );
				} );
		},
		[]( so_5::environment_params_t & params ) {
			params.message_delivery_tracer(
					so_5::msg_tracing::std_cout_tracer() );
		} );

	std::cout << "trace is: " << trace.content() << std::endl;

	const std::string expected{
			"issue-req-1;"
			"mb[1]:pre_invoke;so_based_request{First};mb[1]:post_invoke;"
			"req-1-result=<First>;"
			"issue-req-2;"
			"mb[2]:pre_invoke;user_request{Second};mb[2]:post_invoke;"
			"req-2-result=<Second>;"
	};

	ensure_or_die(
			expected == trace.content(),
			"trace missmatch:\n"
			"expected=" + expected + "\n"
			"  actual=" + trace.content() );
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


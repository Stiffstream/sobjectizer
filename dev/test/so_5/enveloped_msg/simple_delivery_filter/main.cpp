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

struct test_message final
{
	std::string m_a;
	int m_b;
};

class test_case_t final : public so_5::agent_t
{
	so_5::outliving_reference_t< trace_t > m_trace;

	const so_5::mbox_t m_dest;

public :
	test_case_t(
		context_t ctx,
		so_5::outliving_reference_t< trace_t > trace )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_trace{ trace }
		,	m_dest{ special_mbox_t<>::make(
				so_environment().create_mbox(),
				trace,
				"mb" ) }
	{}

	void
	so_define_agent() override
	{
		so_set_delivery_filter( m_dest,
			[]( const test_message & msg ) {
				return "test" != msg.m_a;
			} );

		so_subscribe_self()
			.event( [this]( mhood_t<shutdown> ) {
					m_trace.get().append( "shutdown;" );
					so_deregister_agent_coop_normally();
				} );

		so_subscribe( m_dest )
			.event( [this]( mhood_t<test_message> cmd ) {
					m_trace.get().append( "message=" + cmd->m_a + ";" );
				} );
	}

	void
	so_evt_start() override
	{
		so_5::send< test_message >( m_dest, "first", 0 );
		so_5::send< test_message >( m_dest, "test", 1 );
		so_5::send< test_message >( m_dest, "second", 2 );

		so_5::send< shutdown >( *this );
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

	const std::string expected =
		"mb[1]:inspect;mb[2]:inspect;mb[3]:inspect;"
		"mb[1]:pre_invoke;message=first;mb[1]:post_invoke;"
		"mb[3]:pre_invoke;message=second;mb[3]:post_invoke;"
		"shutdown;";

	ensure_or_die(
			expected == trace.content(),
			"trace missmatch:\n" 
			" expected=" + expected + "\n"
			"   actual=" + trace.content() );
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


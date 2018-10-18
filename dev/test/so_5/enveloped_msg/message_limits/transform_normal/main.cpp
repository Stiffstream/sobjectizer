/*
 * Check for limit_then_transform for a normal case.
 */

#include <so_5/all.hpp>
#include <so_5/h/stdcpp.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

#include <utest_helper_1/h/helper.hpp>

#include "../../common_stuff.hpp"


struct msg_one final : public so_5::message_t {
	std::string m_value;

	msg_one( std::string value ) : m_value( std::move(value) ) {}
};

struct msg_two final : public so_5::message_t {
	std::string m_value;

	msg_two( std::string value ) : m_value( std::move(value) ) {}
};

struct shutdown final : public so_5::signal_t {};

class test_agent_t final : public so_5::agent_t
{
	so_5::outliving_reference_t< trace_t > m_trace;

	const so_5::mbox_t m_mbox;

public:
	test_agent_t(
		context_t ctx,
		so_5::outliving_reference_t< trace_t > trace )
		:	so_5::agent_t{ ctx
				+ limit_then_transform(1,
					[this]( const msg_one & src ) {
						return make_transformed< msg_two >(
								so_direct_mbox(),
								"[" + src.m_value + "]" );
					} )
				+ limit_then_abort<msg_two>(1)
				+ limit_then_abort<shutdown>(1) }
		,	m_trace{ trace }
		,	m_mbox{ special_mbox_t<>::make(
				so_environment().create_mbox(),
				trace,
				"mbox" ) }

	{
		so_default_state().event( m_mbox,
				[this]( mhood_t<msg_one> cmd ) {
					m_trace.get().append( "msg_one{" + cmd->m_value + "};" );
				} )
			.event(
				[this]( mhood_t<msg_two> cmd ) {
					m_trace.get().append( "msg_two{" + cmd->m_value + "};" );
					so_5::send<shutdown>( *this );
				} )
			.event(
				[this]( mhood_t<shutdown> ) {
					so_deregister_agent_coop_normally();
				} );
	}

	void
	so_evt_start() override
	{
		so_5::send<msg_one>( m_mbox, "One" );
		so_5::send<msg_one>( m_mbox, "Two" );
	}
};

void
run_test()
{
	trace_t trace;
	so_5::launch( [&]( so_5::environment_t & env ) {
			env.introduce_coop( [&]( so_5::coop_t & coop ) {
					coop.make_agent< test_agent_t >(
							so_5::outliving_mutable(trace) );
				} );
		},
		[]( so_5::environment_params_t & params ) {
			params.message_delivery_tracer(
					so_5::msg_tracing::std_cout_tracer() );
		} );

	std::cout << "trace is: " << trace.content() << std::endl;

	ensure_or_die(
			"mbox[2]:transform;"
			"mbox[1]:pre_invoke;msg_one{One};mbox[1]:post_invoke;"
			"msg_two{[Two]};"
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


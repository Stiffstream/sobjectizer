/*
 * Check for enveloped service request.
 */

#include <so_5/all.hpp>
#include <so_5/h/stdcpp.hpp>

#include <various_helpers_1/time_limited_execution.hpp>
#include <various_helpers_1/ensure.hpp>

#include <utest_helper_1/h/helper.hpp>

#include "../common_stuff.hpp"

struct so_based_msg final : public so_5::message_t
{
	const std::string m_value;

	so_based_msg( std::string value ) : m_value( std::move(value) ) {}
};

struct user_msg final
{
	std::string m_value;
};

struct simple_signal final : public so_5::message_t {};

struct dummy_msg final {};

void
run_test()
{
	trace_t trace;
	so_5::wrapped_env_t sobj;

	auto mchain = create_mchain( sobj );
	auto special_mbox = special_mbox_t<>::make(
			mchain->as_mbox(),
			so_5::outliving_mutable(trace),
			"mb" );

	so_5::send_delayed<so_based_msg>(
			sobj.environment(),
			special_mbox,
			std::chrono::milliseconds(10),
			"First" );
	so_5::send_delayed<user_msg>(
			sobj.environment(),
			special_mbox,
			std::chrono::milliseconds(15),
			"Second" );
	so_5::send_delayed<simple_signal>(
			sobj.environment(),
			special_mbox,
			std::chrono::milliseconds(20) );
	so_5::send_delayed<dummy_msg>(
			sobj.environment(),
			special_mbox,
			std::chrono::milliseconds(25) );

	auto prepared = prepare_receive(
			from( mchain ).empty_timeout( std::chrono::milliseconds(100) ),
			[&]( so_5::mhood_t<so_based_msg> cmd ) {
				trace.append( "received{" + cmd->m_value + "};" );
			},
			[&]( so_5::mhood_t<user_msg> cmd ) {
				trace.append( "received{" + cmd->m_value + "};" );
			},
			[&]( so_5::mhood_t<simple_signal> ) {
				trace.append( "simple_signal;" );
			} );

	so_5::receive( prepared );

	std::cout << "trace is: " << trace.content() << std::endl;

	const std::string expected{
			"mb[1]:pre_invoke;received{First};mb[1]:post_invoke;"
			"mb[2]:pre_invoke;received{Second};mb[2]:post_invoke;"
			"mb[3]:pre_invoke;simple_signal;mb[3]:post_invoke;"
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


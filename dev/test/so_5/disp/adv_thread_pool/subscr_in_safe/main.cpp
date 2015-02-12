/*
 * A test for adv_thread_pool dispatcher: subscription
 * management methods must throw exception in thread safe event handlers.
 */

#include <iostream>
#include <set>
#include <vector>
#include <exception>
#include <stdexcept>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <sstream>

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

struct msg_shutdown : public so_5::rt::signal_t {};

struct msg_safe_signal : public so_5::rt::signal_t {};

struct msg_unsafe_signal : public so_5::rt::signal_t {};

void
expect_throw(
	const std::string & description,
	int error_code,
	std::function< void() > f )
{
	bool exception_thrown = true;
	try
	{
		f();
		exception_thrown = false;
	}
	catch( const so_5::exception_t & x )
	{
		if( error_code == x.error_code() )
			std::cout << description << ", expected exception: "
				<< x.what() << std::endl;
		else
		{
			std::ostringstream ss;
			ss << description << " expect exception with error_code("
				<< error_code << ", but has: " << x.what();
			throw std::runtime_error( ss.str() );
		}
	}

	if( !exception_thrown )
		throw std::runtime_error(
				"an exception expected for " + description );
}

class a_test_t : public so_5::rt::agent_t
{
	public:
		a_test_t(
			so_5::rt::environment_t & env )
			:	so_5::rt::agent_t( env )
		{
		}

		virtual void
		so_define_agent()
		{
			so_change_state( st_safe );

			so_subscribe_self().in( st_unsafe )
				.event< msg_shutdown >( &a_test_t::evt_shutdown );

			so_subscribe_self().in( st_safe )
				.event< msg_safe_signal >(
						&a_test_t::evt_safe_signal,
						so_5::thread_safe )
				.event< msg_unsafe_signal >( &a_test_t::evt_unsafe_signal );
		}

		void
		so_evt_start()
		{
			so_direct_mbox()->deliver_signal< msg_safe_signal >();

			so_direct_mbox()->deliver_signal< msg_unsafe_signal >();

			so_direct_mbox()->deliver_signal< msg_shutdown >();
		}

		void
		evt_shutdown()
		{
			so_environment().stop();
		}

		void
		evt_safe_signal()
		{
			expect_throw( "so_drop_subscription",
					so_5::rc_operation_enabled_only_on_agent_working_thread,
					[this]() {
						so_drop_subscription( so_direct_mbox(),
								st_safe,
								so_5::signal< msg_safe_signal > );
					} );

			expect_throw( "so_drop_subscription_for_all_states",
					so_5::rc_operation_enabled_only_on_agent_working_thread,
					[this]() {
						so_drop_subscription_for_all_states( so_direct_mbox(),
								so_5::signal< msg_safe_signal > );
					} );

			expect_throw( "so_subscribe",
					so_5::rc_operation_enabled_only_on_agent_working_thread,
					[this]() {
						so_subscribe_self().event< msg_safe_signal >(
								&a_test_t::evt_safe_signal );
					} );
		}

		void
		evt_unsafe_signal()
		{
			so_change_state( st_unsafe );
		}

	private :
		const so_5::rt::state_t st_safe = so_make_state( "safe" );
		const so_5::rt::state_t st_unsafe = so_make_state( "unsafe" );
};

void
run_sobjectizer()
{
	so_5::launch(
		[&]( so_5::rt::environment_t & env )
		{
			env.register_agent_as_coop(
					"test",
					new a_test_t( env ),
					so_5::disp::adv_thread_pool::create_disp_binder(
							"thread_pool",
							so_5::disp::adv_thread_pool::params_t() ) );
		},
		[]( so_5::rt::environment_params_t & params )
		{
			params.add_named_dispatcher(
					"thread_pool",
					so_5::disp::adv_thread_pool::create_disp() );
		} );
}

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				run_sobjectizer();
			},
			5,
			"subscr_in_safe test" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


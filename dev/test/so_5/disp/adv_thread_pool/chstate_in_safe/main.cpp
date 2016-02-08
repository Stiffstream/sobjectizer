/*
 * A test for adv_thread_pool dispatcher: so_change_state
 * must throw exception in thread safe event handlers.
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

#include "../for_each_lock_factory.hpp"

namespace atp_disp = so_5::disp::adv_thread_pool;

struct msg_shutdown : public so_5::signal_t {};

struct msg_safe_signal : public so_5::signal_t {};

struct msg_unsafe_signal : public so_5::signal_t {};

class a_test_t : public so_5::agent_t
{
	public:
		a_test_t(
			so_5::environment_t & env )
			:	so_5::agent_t( env )
		{
		}

		virtual void
		so_define_agent()
		{
			so_change_state( st_safe );

			so_subscribe( so_direct_mbox() ).in( st_unsafe )
				.event< msg_shutdown >( &a_test_t::evt_shutdown );

			so_subscribe( so_direct_mbox() ).in( st_safe )
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
			bool exception_thrown = true;
			try
			{
				so_change_state( st_unsafe );
				exception_thrown = false;
			}
			catch( const so_5::exception_t & x )
			{
				std::cout << "expected exception: " << x.what() << std::endl;
			}

			if( !exception_thrown )
				throw std::runtime_error(
						"an exception on so_change_state expected" );
		}

		void
		evt_unsafe_signal()
		{
			so_change_state( st_unsafe );
		}

	private :
		const so_5::state_t st_safe{ this, "safe" };
		const so_5::state_t st_unsafe{ this, "unsafe" };
};

void
run_sobjectizer( atp_disp::queue_traits::lock_factory_t factory )
{
	so_5::launch(
		[&]( so_5::environment_t & env )
		{
			env.register_agent_as_coop(
					"test",
					new a_test_t( env ),
					atp_disp::create_disp_binder(
							"thread_pool",
							atp_disp::bind_params_t() ) );
		},
		[&]( so_5::environment_params_t & params )
		{
			using namespace atp_disp;
			params.add_named_dispatcher(
					"thread_pool",
					create_disp( disp_params_t{}.set_queue_params(
						queue_traits::queue_params_t{}.lock_factory( factory ) ) ) );
		} );
}

int
main()
{
	try
	{
		for_each_lock_factory( []( atp_disp::queue_traits::lock_factory_t factory ) {
			run_with_time_limit( [&]()
				{
					run_sobjectizer( factory );
				},
				20,
				"chstate_in_safe test" );
		} );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


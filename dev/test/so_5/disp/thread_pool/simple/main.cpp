/*
 * A simple test for thread_pool dispatcher.
 */

#include <iostream>
#include <map>
#include <exception>
#include <stdexcept>
#include <cstdlib>
#include <thread>
#include <chrono>

#include <so_5/rt/h/rt.hpp>
#include <so_5/api/h/api.hpp>

#include <so_5/disp/thread_pool/h/pub.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

struct msg_hello : public so_5::rt::signal_t {};

class a_test_t : public so_5::rt::agent_t
{
	public:
		a_test_t(
			so_5::rt::so_environment_t & env )
			:	so_5::rt::agent_t( env )
		{}

		virtual void
		so_define_agent()
		{
			so_subscribe( so_direct_mbox() ).event(
					so_5::signal< msg_hello >,
					&a_test_t::evt_hello );
		}

		virtual void
		so_evt_start()
		{
			so_direct_mbox()->deliver_signal< msg_hello >();
		}

		void
		evt_hello()
		{
			so_environment().stop();
		}
};

int
main( int argc, char * argv[] )
{
	try
	{
		run_with_time_limit(
			[]()
			{
				so_5::api::run_so_environment(
					[]( so_5::rt::so_environment_t & env )
					{
						env.register_agent_as_coop(
								"test",
								new a_test_t( env ),
								so_5::disp::thread_pool::create_disp_binder(
										"thread_pool",
										so_5::disp::thread_pool::params_t() ) );
					},
					[]( so_5::rt::so_environment_params_t & params )
					{
						params.add_named_dispatcher(
								"thread_pool",
								so_5::disp::thread_pool::create_disp( 4 ) );
					} );
			},
			5,
			"simple thread_pool dispatcher test" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


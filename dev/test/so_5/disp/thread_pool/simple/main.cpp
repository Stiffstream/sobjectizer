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

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

#include "../for_each_lock_factory.hpp"

struct msg_hello : public so_5::signal_t {};

class a_test_t : public so_5::agent_t
{
	public:
		a_test_t(
			so_5::environment_t & env )
			:	so_5::agent_t( env )
		{}

		virtual void
		so_define_agent()
		{
			so_subscribe_self().event< msg_hello >( &a_test_t::evt_hello );
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

void
do_test()
{
	using namespace so_5::disp::thread_pool;
	for_each_lock_factory( []( queue_traits::lock_factory_t factory ) {
		run_with_time_limit( [&]()
			{
				so_5::launch(
					[&]( so_5::environment_t & env )
					{
						auto disp = create_private_disp( env,
								disp_params_t{}
									.thread_count(4)
									.set_queue_params(
										queue_traits::queue_params_t{}
											.lock_factory( factory ) ),
								std::string() );

						env.register_agent_as_coop(
								"test",
								new a_test_t( env ),
								disp->binder( bind_params_t{} ) );
					} );
			},
			20,
			"simple thread_pool dispatcher test" );
	} );
}

int
main()
{
	try
	{
		do_test();
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


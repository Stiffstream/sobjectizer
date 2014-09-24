/*
 * A simple test for chaining thread safe service handlers in
 * adv_thread_pool dispatcher.
 */

#include <iostream>
#include <map>
#include <exception>
#include <stdexcept>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <sstream>

#include <so_5/rt/h/rt.hpp>
#include <so_5/api/h/api.hpp>

#include <so_5/disp/adv_thread_pool/h/pub.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

const unsigned int thread_pool_size = 16;

struct msg_run_test : public so_5::rt::signal_t {};

struct msg_request_string : public so_5::rt::message_t
{
	unsigned int m_request_number;

	msg_request_string( unsigned int number )
		:	m_request_number( number )
		{}
};

class a_test_t : public so_5::rt::agent_t
{
	public:
		a_test_t(
			so_5::rt::environment_t & env )
			:	so_5::rt::agent_t( env )
		{}

		virtual void
		so_define_agent()
		{
			so_subscribe( so_direct_mbox() ).event(
					so_5::signal< msg_run_test >,
					&a_test_t::evt_run_test,
					so_5::thread_safe );

			so_subscribe( so_direct_mbox() ).event(
					&a_test_t::evt_request_string,
					so_5::thread_safe );
		}

		virtual void
		so_evt_start()
		{
			so_direct_mbox()->deliver_signal< msg_run_test >();
		}

		void
		evt_run_test()
		{
			std::ostringstream ss;
			for( unsigned int i = 1; i < thread_pool_size; ++i )
				ss << i << ":";

			const std::string expected = ss.str();

			const std::string actual = do_request_string( 1 );

			if( expected != actual )
				throw std::runtime_error( "Value mismatch! expected: '" +
						expected + "', actual: '" + actual + "'" );

			so_environment().stop();
		}

		std::string
		evt_request_string( const msg_request_string & evt )
		{
			std::ostringstream ss;
			ss << evt.m_request_number << ":";

			if( evt.m_request_number + 1 < thread_pool_size )
				return ss.str() + do_request_string( evt.m_request_number + 1 );
			else
				return ss.str();
		}

	private :
		std::string
		do_request_string( unsigned int number )
		{
			return so_direct_mbox()->get_one< std::string >()
					.wait_forever().sync_get( new msg_request_string( number ) );
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
				so_5::launch(
					[]( so_5::rt::environment_t & env )
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
								so_5::disp::adv_thread_pool::create_disp( 
										thread_pool_size ) );
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


/*
 * A simple test for chaining thread safe service handlers in
 * adv_thread_pool dispatcher.
 * Ad-hoc agents are used.
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

so_5::rt::agent_coop_unique_ptr_t
create_test_coop( so_5::rt::so_environment_t & env )
{
	auto c = env.create_coop( "test",
			so_5::disp::adv_thread_pool::create_disp_binder(
					"thread_pool",
					so_5::disp::adv_thread_pool::params_t() ) );

	auto mbox = env.create_local_mbox();
	auto requester = [mbox]( unsigned int number ) {
			return mbox->get_one< std::string >()
					.wait_forever().sync_get( new msg_request_string( number ) );
		};

	c->define_agent()
		.on_start( [mbox]() {
				mbox->deliver_signal< msg_run_test >();
			} )
		.event( mbox, so_5::signal< msg_run_test >,
			[&env, mbox, requester]() {
				std::ostringstream ss;
				for( unsigned int i = 1; i < thread_pool_size; ++i )
					ss << i << ":";

				const std::string expected = ss.str();

				const std::string actual = requester( 1 );

				if( expected != actual )
					throw std::runtime_error( "Value mismatch! expected: '" +
							expected + "', actual: '" + actual + "'" );

				env.stop();
			},
			so_5::thread_safe )
		.event( mbox,
			[mbox, requester]( const msg_request_string & evt ) {
				std::ostringstream ss;
				ss << evt.m_request_number << ":";

				if( evt.m_request_number + 1 < thread_pool_size )
					return ss.str() + requester( evt.m_request_number + 1 );
				else
					return ss.str();
			},
			so_5::thread_safe );

	return c;
}

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
						env.register_coop( create_test_coop( env ) );
					},
					[]( so_5::rt::so_environment_params_t & params )
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


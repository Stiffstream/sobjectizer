/*
 * Test of scheduling timer with zero milliseconds delay.
 */

#include <iostream>
#include <cstdlib>
#include <stdexcept>

#include <so_5/all.hpp>

struct msg_test : public so_5::message_t
{};

struct msg_do_resend : public so_5::signal_t
{};

struct msg_stop : public so_5::signal_t
{};

class a_test_t : public so_5::agent_t
{
		typedef so_5::agent_t base_type_t;

	public :
		a_test_t(
			so_5::environment_t & env,
			int & message_counter )
			:	base_type_t( env )
			,	m_message_counter( message_counter )
			,	m_mbox( env.create_mbox() )
		{
		}

		void
		so_define_agent()
		{
			so_subscribe( m_mbox )
				.event( &a_test_t::evt_delayed_message );

			so_subscribe( m_mbox )
				.event( &a_test_t::evt_do_resend );

			so_subscribe( m_mbox )
				.event( &a_test_t::evt_stop );
		}

		void
		so_evt_start()
		{
			so_environment().single_timer< msg_stop >( m_mbox, 1000 );

			so_environment().single_timer(
					std::unique_ptr< msg_test >( new msg_test() ), m_mbox, 0 );

			m_mbox->deliver_signal< msg_do_resend >();
		}

		void
		evt_delayed_message( const so_5::event_data_t< msg_test > & )
		{
			m_message_counter += 1;
		}

		void
		evt_do_resend( const so_5::event_data_t< msg_do_resend > & )
		{
			so_environment().single_timer(
					std::unique_ptr< msg_test >( new msg_test() ), m_mbox, 0 );
		}

		void
		evt_stop( const so_5::event_data_t< msg_stop > & )
		{
			so_environment().stop();
		}

	private :
		int & m_message_counter;

		so_5::mbox_t m_mbox;
};

struct test_env_t
{
	int m_message_counter;

	test_env_t()
		:	m_message_counter( 0 )
	{}

	void
	init( so_5::environment_t & env )
	{
		env.register_agent_as_coop( "test_coop",
				new a_test_t( env, m_message_counter ) );
	}
};

int
main()
{
	try
	{
		test_env_t test_env;

		so_5::launch(
			[&]( so_5::environment_t & env )
			{
				test_env.init( env );
			} );

		if( 2 != test_env.m_message_counter )
		{
			std::cerr << "expected and actual message_counter value mismatch, "
					"actual value: " << test_env.m_message_counter << std::endl;
			std::abort();
		}

		return 0;
	}
	catch( const std::exception & x )
	{
		std::cerr << "Exception caught: " << x.what() << std::endl;
	}

	return 2;
}



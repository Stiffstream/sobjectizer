/*
 * A test for processing drop_subscription when demand is in queue.
 */

#include <iostream>
#include <cstdlib>
#include <sstream>

#include <so_5/all.hpp>

struct msg_one : public so_5::signal_t {};
struct msg_two : public so_5::signal_t {};
struct msg_three : public so_5::signal_t {};
struct msg_four : public so_5::signal_t {};

class a_test_t : public so_5::agent_t
{
		typedef so_5::agent_t base_type_t;

	public :
		a_test_t(
			so_5::environment_t & env )
			:	base_type_t( env )
			,	m_mbox( env.create_mbox() )
		{
		}

		void
		so_define_agent()
		{
			so_subscribe( m_mbox )
				.event( &a_test_t::evt_one );
			so_subscribe( m_mbox )
				.event( &a_test_t::evt_two );
			so_subscribe( m_mbox )
				.event( &a_test_t::evt_three );
			so_subscribe( m_mbox )
				.event( &a_test_t::evt_four );
		}

		void
		so_evt_start()
		{
			m_mbox->deliver_signal< msg_one >();
			m_mbox->deliver_signal< msg_two >();
			m_mbox->deliver_signal< msg_three >();
		}

		void
		evt_one( const so_5::event_data_t< msg_one > & )
		{
			for( size_t i = 0; i != 10000u; ++i )
			{
				so_drop_subscription( m_mbox, &a_test_t::evt_two );

				so_subscribe( m_mbox ).event( &a_test_t::evt_two );

				m_mbox->deliver_signal< msg_two >();
			}

			so_drop_subscription( m_mbox, &a_test_t::evt_two );
		}

		void
		evt_two( const so_5::event_data_t< msg_two > & )
		{
			std::abort();
		}

		void
		evt_three( const so_5::event_data_t< msg_three > & )
		{
			m_mbox->deliver_signal< msg_four >();
		}

		void
		evt_four( const so_5::event_data_t< msg_four > & )
		{
			so_environment().stop();
		}

	private :
		so_5::mbox_t m_mbox;
};

void
init( so_5::environment_t & env )
{
	env.register_agent_as_coop( "test", new a_test_t( env ) );
}

int
main()
{
	try
	{
		so_5::launch( &init );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

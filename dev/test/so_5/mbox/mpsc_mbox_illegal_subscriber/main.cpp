/*
 * A test for mpsc_mbox: subscription form another agent must
 * lead to exception.
 */

#include <iostream>
#include <sstream>

#include <so_5/all.hpp>

struct msg_one : public so_5::signal_t {};

class a_first_t : public so_5::agent_t
{
	public :
		a_first_t(
			so_5::environment_t & env )
			:	so_5::agent_t( env )
		{
		}

		void
		so_define_agent()
		{
			so_subscribe( so_direct_mbox() ).event( &a_first_t::evt_one );
		}

		void
		so_evt_start()
		{
			so_environment().stop();
		}

		void
		evt_one( const so_5::event_data_t< msg_one > & )
		{}
};

class a_second_t : public so_5::agent_t
{
	public :
		a_second_t(
			so_5::environment_t & env,
			const so_5::mbox_t & mbox )
			:	so_5::agent_t( env )
			,	m_mbox( mbox )
		{
		}

		void
		so_define_agent()
		{
			try
			{
				so_subscribe( m_mbox ).event( &a_second_t::evt_one );
			}
			catch( const so_5::exception_t & x )
			{
				if( so_5::rc_illegal_subscriber_for_mpsc_mbox != x.error_code() )
					throw;
			}
		}

		void
		evt_one( const so_5::event_data_t< msg_one > & )
		{}

	private :
		const so_5::mbox_t m_mbox;
};

int
main()
{
	try
	{
		so_5::launch(
			[]( so_5::environment_t & env )
			{
				auto coop = env.create_coop( "test" );

				auto a_first = new a_first_t( env );
				coop->add_agent( a_first );

				coop->add_agent( new a_second_t( env, a_first->so_direct_mbox() ) );

				env.register_coop( std::move( coop ) );
			} );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


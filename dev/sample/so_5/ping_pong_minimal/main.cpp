#include <iostream>

#include <so_5/all.hpp>

// Types of signals for the agents.
struct msg_ping : public so_5::rt::signal_t {};
struct msg_pong : public so_5::rt::signal_t {};

// Class of pinger agent.
class a_pinger_t : public so_5::rt::agent_t
	{
	public :
		a_pinger_t(
			so_5::rt::environment_t & env,
			const so_5::rt::mbox_t & mbox,
			int pings_to_send )
			:	so_5::rt::agent_t( env )
			,	m_mbox( mbox )
			,	m_pings_left( pings_to_send )
			{}

		virtual void
		so_define_agent() override
			{
				so_default_state().event< msg_pong >(
						m_mbox, &a_pinger_t::evt_pong );
			}

		virtual void
		so_evt_start() override
			{
				send_ping();
			}

		void
		evt_pong()
			{
				if( m_pings_left > 0 )
					send_ping();
				else
					so_environment().stop();
			}

	private :
		const so_5::rt::mbox_t m_mbox;

		int m_pings_left;

		void
		send_ping()
		{
			m_mbox->deliver_signal< msg_ping >();
			--m_pings_left;
		}
	};

int
main()
{
	try
	{
		so_5::launch(
			[]( so_5::rt::environment_t & env )
			{
				// Mbox for agent's interaction.
				auto mbox = env.create_local_mbox();

				// Agent's cooperation.
				auto coop = env.create_coop( "ping_pong" );

				// Pinger.
				coop->add_agent( new a_pinger_t( env, mbox, 100000 ) );

				// Ponger agent.
				coop->define_agent().event< msg_ping >(
						mbox, [mbox]() { so_5::send< msg_pong >( mbox ); } );

				// Register the cooperation.
				env.register_coop( std::move( coop ) );
			} );

		return 0;
	}
	catch( const std::exception & x )
	{
		std::cerr << "*** Exception caught: " << x.what() << std::endl;
	}

	return 2;
}


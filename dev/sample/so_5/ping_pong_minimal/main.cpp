#include <iostream>

#include <so_5/all.hpp>

// Types of signals for the agents.
struct msg_ping final : public so_5::signal_t {};
struct msg_pong final : public so_5::signal_t {};

// Class of pinger agent.
class a_pinger_t final : public so_5::agent_t
	{
	public :
		a_pinger_t( context_t ctx, so_5::mbox_t mbox, int pings_to_send )
			:	so_5::agent_t{ ctx }
			,	m_mbox{ std::move(mbox) }
			,	m_pings_left{ pings_to_send }
			{}

		void so_define_agent() override
			{
				so_subscribe( m_mbox ).event( &a_pinger_t::evt_pong );
			}

		void so_evt_start() override
			{
				send_ping();
			}

	private :
		const so_5::mbox_t m_mbox;

		int m_pings_left;

		void evt_pong(mhood_t< msg_pong >)
			{
				if( m_pings_left > 0 )
					send_ping();
				else
					so_environment().stop();
			}

		void send_ping()
			{
				so_5::send< msg_ping >( m_mbox );
				--m_pings_left;
			}
	};

class a_ponger_t final : public so_5::agent_t
	{
	public :
		a_ponger_t( context_t ctx, const so_5::mbox_t & mbox )
			:	so_5::agent_t( std::move(ctx) )
		{
			so_subscribe( mbox ).event(
				[mbox]( mhood_t<msg_ping> ) {
					so_5::send< msg_pong >( mbox );
				} );
		}
	};

int main()
	{
		try
			{
				so_5::launch( []( so_5::environment_t & env ) {
						env.introduce_coop( [&env]( so_5::coop_t & coop ) {
							// Mbox for agent's interaction.
							auto mbox = env.create_mbox();

							// Pinger.
							coop.make_agent< a_pinger_t >( mbox, 100000 );

							// Ponger agent.
							coop.make_agent< a_ponger_t >( std::cref(mbox) );
						});
					});

				return 0;
			}
		catch( const std::exception & x )
			{
				std::cerr << "*** Exception caught: " << x.what() << std::endl;
			}

		return 2;
	}


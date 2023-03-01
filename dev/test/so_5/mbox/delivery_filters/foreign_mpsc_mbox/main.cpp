/*
 * A test for mpsc_mbox: setting a delivery filter form another agent must
 * lead to exception.
 */

#include <iostream>
#include <sstream>

#include <so_5/all.hpp>

struct msg_demo : public so_5::message_t {};

class a_first_t : public so_5::agent_t
{
	public :
		a_first_t(
			so_5::environment_t & env )
			:	so_5::agent_t( env )
		{
		}

		void
		so_define_agent() override
		{
			so_set_delivery_filter( so_direct_mbox(),
					[]( const msg_demo & ) { return true; } );

			so_subscribe( so_direct_mbox() ).event( &a_first_t::evt_demo );
		}

		void
		so_evt_start() override
		{
			so_environment().stop();
		}

		void
		evt_demo( mhood_t< msg_demo > )
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
		so_define_agent() override
		{
			try
			{
				so_set_delivery_filter( m_mbox,
						[]( const msg_demo & ) { return true; } );

				// Shouldn't be here!
				throw std::runtime_error{ "successful so_set_delivery_filter completion!" };
			}
			catch( const so_5::exception_t & x )
			{
				if( so_5::rc_illegal_subscriber_for_mpsc_mbox != x.error_code() )
					throw;
			}
		}

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
				auto coop = env.make_coop();

				auto a_first = coop->make_agent< a_first_t >();

				coop->make_agent< a_second_t >( a_first->so_direct_mbox() );

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


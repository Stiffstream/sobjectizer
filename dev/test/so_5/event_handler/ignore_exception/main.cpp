/*
 * A test for checking so_5::ignore_exception behaviour.
 */

#include <iostream>
#include <stdexcept>

#include <so_5/all.hpp>

struct msg_test_signal : public so_5::signal_t {};

class a_test_t
	:	public so_5::agent_t
{
		typedef so_5::agent_t base_type_t;

	public :
		a_test_t(
			so_5::environment_t & env )
			:	base_type_t( env )
			,	m_self_mbox( env.create_mbox() )
			,	m_counter( 0 )
			,	m_max_attempts( 3 )
		{}

		void
		so_define_agent()
		{
			so_subscribe( m_self_mbox ).event( &a_test_t::evt_signal );
		}

		virtual void
		so_evt_start()
		{
			m_self_mbox->deliver_signal< msg_test_signal >();
		}

		void
		evt_signal( const so_5::event_data_t< msg_test_signal > & )
		{
			if( m_counter < m_max_attempts )
			{
				m_counter += 1;
				m_self_mbox->deliver_signal< msg_test_signal >();

				throw std::runtime_error( "Another exception from evt_signal" );
			}
			else
				so_environment().stop();
		}

		virtual so_5::exception_reaction_t
		so_exception_reaction() const
		{
			return so_5::ignore_exception;
		}

	private :
		const so_5::mbox_t m_self_mbox;

		int m_counter;
		const int m_max_attempts;
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


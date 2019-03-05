/*
 * A test for delayed message.
 */

#include <iostream>
#include <map>
#include <exception>
#include <stdexcept>

#include <so_5/all.hpp>

struct test_message
	: public so_5::signal_t
{};

class test_agent_t
	:
		public so_5::agent_t
{
		typedef so_5::agent_t base_type_t;

	public:

		test_agent_t(
			so_5::environment_t & env )
			:
				base_type_t( env ),
				m_test_mbox( so_environment().create_mbox() )
		{
		}

		virtual ~test_agent_t()
		{
		}

		virtual void
		so_define_agent();

		virtual void
		so_evt_start()
		{
			// Send a delayed for 1 second message.
			m_timer_ref = so_5::send_periodic< test_message >(
					m_test_mbox,
					std::chrono::seconds(1),
					std::chrono::seconds(0) );
		}

		void
		evt_test( mhood_t< test_message > );

	private:
		so_5::mbox_t	m_test_mbox;

		so_5::timer_thread::timer_id_ref_t	m_timer_ref;

};

void
test_agent_t::so_define_agent()
{
	so_subscribe( m_test_mbox )
		.event( &test_agent_t::evt_test );
}

void
test_agent_t::evt_test(mhood_t< test_message >)
{
	so_environment().stop();
}

void
init( so_5::environment_t & env )
{
	env.register_agent_as_coop( "test_coop", env.make_agent< test_agent_t >() );
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


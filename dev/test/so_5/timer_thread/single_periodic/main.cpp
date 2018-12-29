/*
 * A test for scheduling and canceling delayed/periodic message.
*/

#include <iostream>
#include <map>
#include <exception>
#include <stdexcept>

#include <so_5/all.hpp>

struct test_message
	: public so_5::signal_t
{};

struct stop_message
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
			// Schedule periodic message.
			m_timer_ref = so_environment().schedule_timer< test_message >(
				m_test_mbox,
				300,
				200 );
		}

		void
		evt_test(
			const so_5::event_data_t< test_message > & msg );

		void
		evt_stop(
			const so_5::event_data_t< stop_message > & msg );

		static int	m_evt_count;
		static const int m_test_evt_count;
	private:

		so_5::mbox_t	m_test_mbox;

		so_5::timer_thread::timer_id_ref_t	m_timer_ref;

};

int	test_agent_t::m_evt_count = 0;
const int test_agent_t::m_test_evt_count = 5;


void
test_agent_t::so_define_agent()
{
	so_subscribe( m_test_mbox )
		.event( &test_agent_t::evt_test );

	so_subscribe( m_test_mbox )
		.event( &test_agent_t::evt_stop );
}

void
test_agent_t::evt_test(
	const so_5::event_data_t< test_message > & )
{
	if( m_test_evt_count == ++m_evt_count )
	{
		// Reschedule message. Old timer event should be released.
		m_timer_ref = 
			so_environment().schedule_timer< stop_message >(
				m_test_mbox,
				800,
				0 );
	}
}
void
test_agent_t::evt_stop(
	const so_5::event_data_t< stop_message > & )
{
	so_environment().stop();
}

void
init( so_5::environment_t & env )
{
	env.register_agent_as_coop( "test_coop", new test_agent_t( env ) );
}

int
main()
{
	try
	{
		so_5::launch( &init );

		if( test_agent_t::m_test_evt_count != test_agent_t::m_evt_count )
		{
			std::cerr << "test_agent_t::m_test_evt_count ="
				<< test_agent_t::m_test_evt_count << "\n"
				<< "test_agent_t::m_evt_count = "
				<< test_agent_t::m_evt_count
				<< std::endl;

			throw std::runtime_error(
				"test_agent_t::m_test_evt_count != test_agent_t::m_evt_count" );
		}
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}




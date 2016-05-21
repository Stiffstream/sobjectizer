/*
 * A test for event subscription and unsubsription.
 */

#include <iostream>
#include <cstdlib>
#include <exception>

#include <so_5/all.hpp>

#define PERROR_AND_ABORT_IMPL(file, line, condition_text) \
	std::cerr << file << ":" << line << ": exception expected but not thrown: " << condition_text << std::endl; \
	std::abort();

#define ENSURE_EXCEPTION(condition) \
{ \
	bool exception_thrown = false; \
	try { condition; } \
	catch( const so_5::exception_t & ) { exception_thrown = true; } \
	if( !exception_thrown ) \
	{ \
		PERROR_AND_ABORT_IMPL(__FILE__, __LINE__, #condition ) \
	} \
}

struct msg_test : public so_5::signal_t {};

struct msg_stop : public so_5::signal_t {};

class test_agent_t
	:
		public so_5::agent_t
{
		typedef so_5::agent_t base_type_t;

		const so_5::state_t m_state_a{ this };
		const so_5::state_t m_state_b{ this };

	public:
		test_agent_t(
			so_5::environment_t & env )
			:
				base_type_t( env ),
				m_mbox( so_environment().create_mbox() )
		{}

		virtual ~test_agent_t()
		{}

		virtual void
		so_define_agent();

		virtual void
		so_evt_start();

		void
		evt_handler1(
			const so_5::event_data_t< msg_test > & )
		{
			// Should be called in default state.
		}

		void
		evt_handler3(
			const so_5::event_data_t< msg_test > & )
		{
			std::cerr << "Error: evt_handler3 called..." << std::endl;
			std::abort();
		}

		void
		evt_stop(
			const so_5::event_data_t< msg_stop > & )
		{
			so_environment().stop();
		}

	private:
		// Mbox to subscribe.
		so_5::mbox_t m_mbox;
};


void
test_agent_t::so_define_agent()
{
	so_subscribe( m_mbox )
		.event( &test_agent_t::evt_stop );
}

void
test_agent_t::so_evt_start()
{
	// Subscribe by one handler.
	// Then trying to subscribe by another handler. Error expected.
	so_subscribe( m_mbox ).event( &test_agent_t::evt_handler1 );
	so_subscribe( m_mbox ).in( m_state_a ).event(
			&test_agent_t::evt_handler1 );
	so_subscribe( m_mbox ).in( m_state_b ).event(
			&test_agent_t::evt_handler3 );

	ENSURE_EXCEPTION( so_subscribe( m_mbox ).event(
			&test_agent_t::evt_handler1 ) );
	ENSURE_EXCEPTION( so_subscribe( m_mbox ).event(
			&test_agent_t::evt_handler3 ) );

	ENSURE_EXCEPTION( so_subscribe( m_mbox ).in( m_state_a ).event(
			&test_agent_t::evt_handler1 ) );
	ENSURE_EXCEPTION( so_subscribe( m_mbox ).in( m_state_a ).event(
			&test_agent_t::evt_handler3 ) );

	ENSURE_EXCEPTION( so_subscribe( m_mbox ).in( m_state_b ).event(
			&test_agent_t::evt_handler1 ) );
	ENSURE_EXCEPTION( so_subscribe( m_mbox ).in( m_state_b ).event(
			&test_agent_t::evt_handler3 ) );

	m_mbox->deliver_signal< msg_test >();
	m_mbox->deliver_signal< msg_stop >();
}

void
init(
	so_5::environment_t & env )
{
	env.register_agent_as_coop(
			"test_coop",
			new test_agent_t( env ) );
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


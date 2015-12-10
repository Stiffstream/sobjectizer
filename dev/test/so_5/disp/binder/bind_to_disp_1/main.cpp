/*
 * A test for dispatcher binders.
 *
 * A cooperation is registered. Agents of cooperation is
 * bound to different dispatchers.
 */

#include <iostream>
#include <exception>
#include <stdexcept>
#include <memory>

#include <so_5/all.hpp>

// Count of messages to be sent at once.
const unsigned int g_send_at_once = 10;

// A count of message bunches.
const unsigned int g_send_session_count = 100;

struct test_message
	:
		public so_5::message_t
{
	test_message(): m_is_last( false ) {}
	virtual ~test_message() {}

	bool m_is_last;
};

// A signal to start sending.
struct send_message_signal
	: public so_5::signal_t
{};

class test_agent_sender_t
	:
		public so_5::agent_t
{
		typedef so_5::agent_t base_type_t;

	public:

		test_agent_sender_t(
			so_5::environment_t & env,
			const so_5::mbox_t & mbox )
			:
				base_type_t( env ),
				m_send_session_complited( 0 ),
				m_mbox_receiver( mbox ),
				m_notification_mbox( so_environment().create_mbox() )
		{}

		virtual ~test_agent_sender_t()
		{}

		virtual void
		so_define_agent();

		virtual void
		so_evt_start();

		void
		evt_send_messages(
			const so_5::event_data_t< send_message_signal > &
				msg );

	private:
		// A counter of bunch sent.
		unsigned int m_send_session_complited;

		// Receiver mbox.
		so_5::mbox_t m_mbox_receiver;

		// Self mbox.
		so_5::mbox_t m_notification_mbox;
};

void
test_agent_sender_t::so_define_agent()
{
	so_subscribe( m_notification_mbox )
		.event( &test_agent_sender_t::evt_send_messages );
}

void
test_agent_sender_t::so_evt_start()
{
	m_notification_mbox->deliver_signal< send_message_signal >();
}

void
test_agent_sender_t::evt_send_messages(
	const so_5::event_data_t< send_message_signal > & )
{
	for( unsigned int i = 0; i < g_send_at_once; ++i )
	{
		m_mbox_receiver->deliver_message( new test_message() );
	}

	++m_send_session_complited;

	// If all bunches are sent then final message should be sent.
	if( g_send_session_count <= m_send_session_complited )
	{
		std::unique_ptr< test_message > tm( new test_message );
		tm->m_is_last = true;

		m_mbox_receiver->deliver_message( std::move(tm) );
	}
	else
	{
		m_notification_mbox->deliver_signal< send_message_signal >();
	}
}

class test_agent_receiver_t
	:
		public so_5::agent_t
{
		typedef so_5::agent_t base_type_t;

	public:
		test_agent_receiver_t(
			so_5::environment_t & env,
			const so_5::mbox_t & mbox )
			:
				base_type_t( env ),
				m_source_mbox( mbox )
		{}

		virtual ~test_agent_receiver_t()
		{}

		virtual void
		so_define_agent();

		virtual void
		so_evt_start()
		{}

		void
		evt_test(
			const so_5::event_data_t< test_message > &
				msg );

	private:
		// A source of messages.
		so_5::mbox_t m_source_mbox;
};

void
test_agent_receiver_t::so_define_agent()
{
	so_subscribe( m_source_mbox )
		.event( &test_agent_receiver_t::evt_test );
}

void
test_agent_receiver_t::evt_test(
	const so_5::event_data_t< test_message > & msg )
{
	// Stop if this is the last message.
	if( msg->m_is_last )
		so_environment().stop();
}

void
init( so_5::environment_t & env )
{
	so_5::mbox_t mbox = env.create_mbox();

	so_5::coop_unique_ptr_t coop =
		env.create_coop( "test_coop" );

	coop->add_agent(
		new test_agent_sender_t( env, mbox ),
		so_5::disp::one_thread::create_disp_binder(
			"sender_disp" ) );

	coop->add_agent(
		new test_agent_receiver_t( env, mbox ),
		so_5::disp::one_thread::create_disp_binder(
			"receiver_disp" ) );

	env.register_coop( std::move( coop ) );
}

int
main()
{
	try
	{
		so_5::launch(
			&init,
			[]( so_5::environment_params_t & params ) {
					params.add_named_dispatcher(
						"sender_disp",
						so_5::disp::one_thread::create_disp() );
					params.add_named_dispatcher(
						"receiver_disp",
						so_5::disp::one_thread::create_disp() );
			} );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

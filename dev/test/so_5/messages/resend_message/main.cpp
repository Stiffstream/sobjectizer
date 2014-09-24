/*
 * Test of resending the same message.
 */

#include <iostream>
#include <stdexcept>

#include <so_5/all.hpp>

struct msg_test;

class controller_t
{
	private :
		enum state_t
		{
			INITIAL,
			MSG_CREATED,
			MSG_SEND_1,
			MSG_RECEIVE_1,
			MSG_SEND_2,
			MSG_RECEIVE_2,
			MSG_DESTROYED
		};

		state_t m_state;

		const msg_test * m_msg_ptr;

		static const char *
		state_name( state_t state )
		{
			switch( state )
			{
				case INITIAL: return "INITIAL";
				case MSG_CREATED: return "MSG_CREATED";
				case MSG_SEND_1: return "MSG_SEND_1";
				case MSG_RECEIVE_1: return "MSG_RECEIVE_1";
				case MSG_SEND_2: return "MSG_SEND_2";
				case MSG_RECEIVE_2: return "MSG_RECEIVE_2";
				case MSG_DESTROYED: return "MSG_DESTROYED";
			}

			throw std::runtime_error( "unknown state!" );
		}

		void
		ensure_valid_state(
			state_t expected_state ) const
		{
			if( m_state != expected_state )
			{
				std::cerr << "expected and actual states mismatch! "
							"current state: "
						<< state_name( m_state ) << " expected state: "
						<< state_name( expected_state );

				std::abort();
			}
		}

		void
		ensure_valid_msg_ptr(
			const msg_test * ptr )
		{
			if( m_msg_ptr != ptr )
			{
				std::cerr << "message pointers mismatch!";
				std::abort();
			}
		}

	public :
		controller_t()
			:	m_state( INITIAL )
			,	m_msg_ptr( nullptr )
		{}

		void
		msg_created( const msg_test * ptr )
		{
			ensure_valid_state( INITIAL );
			m_state = MSG_CREATED;
			m_msg_ptr = ptr;
		}

		void
		msg_send_1( const msg_test * ptr )
		{
			ensure_valid_state( MSG_CREATED );
			ensure_valid_msg_ptr( ptr );

			m_state = MSG_SEND_1;
		}

		void
		msg_receive_1( const msg_test * ptr )
		{
			ensure_valid_state( MSG_SEND_1 );
			ensure_valid_msg_ptr( ptr );

			m_state = MSG_RECEIVE_1;
		}

		void
		msg_send_2( const msg_test * ptr )
		{
			ensure_valid_state( MSG_RECEIVE_1 );
			ensure_valid_msg_ptr( ptr );

			m_state = MSG_SEND_2;
		}

		void
		msg_receive_2( const msg_test * ptr )
		{
			ensure_valid_state( MSG_SEND_2 );
			ensure_valid_msg_ptr( ptr );

			m_state = MSG_RECEIVE_2;
		}

		void
		msg_destroyed( const msg_test * ptr )
		{
			ensure_valid_state( MSG_RECEIVE_2 );
			ensure_valid_msg_ptr( ptr );

			m_state = MSG_DESTROYED;
			m_msg_ptr = nullptr;
		}

		void
		ensure_test_passed()
		{
			ensure_valid_state( MSG_DESTROYED );
			ensure_valid_msg_ptr( nullptr );
		}
};

struct msg_test : public so_5::rt::message_t
{
	msg_test( controller_t & controller )
		:	m_controller( controller )
	{
		m_controller.msg_created( this );
	}

	virtual ~msg_test()
	{
		m_controller.msg_destroyed( this );
	}

	controller_t & m_controller;
};

struct msg_stop : public so_5::rt::signal_t
{};

class a_test_t : public so_5::rt::agent_t
{
		typedef so_5::rt::agent_t base_type_t;

	public :
		a_test_t(
			so_5::rt::environment_t & env,
			controller_t & controller )
			:	base_type_t( env )
			,	m_controller( controller )
			,	m_mbox_1( env.create_local_mbox() )
			,	m_mbox_2( env.create_local_mbox() )
		{
		}

		void
		so_define_agent()
		{
			so_subscribe( m_mbox_1 )
				.event( &a_test_t::evt_msg_1 );

			so_subscribe( m_mbox_2 )
				.event( &a_test_t::evt_msg_2 );

			so_subscribe( m_mbox_2 )
				.event( &a_test_t::evt_stop );
		}

		void
		so_evt_start()
		{
			std::unique_ptr< msg_test > msg( new msg_test( m_controller ) );

			m_controller.msg_send_1( msg.get() );

			m_mbox_1->deliver_message( std::move(msg) );
		}

		void
		evt_msg_1( const so_5::rt::event_data_t< msg_test > & evt )
		{
			m_controller.msg_receive_1( evt.get() );

			auto ref = evt.make_reference();
			m_controller.msg_send_2( ref.get() );

			m_mbox_2->deliver_message( ref );
		}

		void
		evt_msg_2( const so_5::rt::event_data_t< msg_test > & evt )
		{
			m_controller.msg_receive_2( evt.get() );

			m_mbox_2->deliver_signal< msg_stop >();
		}

		void
		evt_stop( const so_5::rt::event_data_t< msg_stop > & )
		{
			so_environment().stop();
		}

	private :
		controller_t & m_controller;

		so_5::rt::mbox_ref_t m_mbox_1;
		so_5::rt::mbox_ref_t m_mbox_2;
};

struct test_env_t
{
	controller_t m_controller;

	void
	init( so_5::rt::environment_t & env )
	{
		env.register_agent_as_coop( "test_coop",
				new a_test_t( env, m_controller ) );
	}
};

int
main( int argc, char ** argv )
{
	try
	{
		test_env_t test_env;

		so_5::launch(
			[&]( so_5::rt::environment_t & env )
			{
				test_env.init( env );
			} );

		test_env.m_controller.ensure_test_passed();

		return 0;
	}
	catch( const std::exception & x )
	{
		std::cerr << "Exception caught: " << x.what() << std::endl;
	}

	return 2;
}


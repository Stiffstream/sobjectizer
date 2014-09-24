/*
 * A test for so_drop_subscription methods.
 */

#include <iostream>
#include <sstream>

#include <so_5/all.hpp>

struct msg_one : public so_5::rt::signal_t {};
struct msg_two : public so_5::rt::signal_t {};
struct msg_three : public so_5::rt::signal_t {};

struct msg_four : public so_5::rt::message_t {};

struct msg_five : public so_5::rt::signal_t {};

class a_test_t : public so_5::rt::agent_t
{
		typedef so_5::rt::agent_t base_type_t;

	public :
		a_test_t(
			so_5::rt::environment_t & env,
			std::string & sequence )
			:	base_type_t( env )
			,	m_sequence( sequence )
			,	m_mbox( env.create_local_mbox() )
		{
		}

		void
		so_define_agent()
		{
			so_subscribe( m_mbox )
				.event( &a_test_t::evt_default_one )
				.event( &a_test_t::evt_default_two )
				.event( &a_test_t::evt_default_three )
				.event( &a_test_t::evt_default_four )
				.event( so_5::signal< msg_five >, &a_test_t::evt_five );

			so_subscribe( m_mbox ).in( st_1 )
				.event( &a_test_t::evt_st_1_one )
				.event( &a_test_t::evt_st_1_two )
				.event( &a_test_t::evt_st_1_three )
				.event( &a_test_t::evt_default_four )
				.event( so_5::signal< msg_five >, &a_test_t::evt_five );

			so_subscribe( m_mbox ).in( st_2 )
				.event( &a_test_t::evt_st_2_one )
				.event( &a_test_t::evt_st_2_two )
				.event( &a_test_t::evt_st_2_three )
				.event( so_5::signal< msg_five >, &a_test_t::evt_five );
		}

		void
		so_evt_start()
		{
			m_mbox->deliver_signal< msg_one >();
			m_mbox->deliver_signal< msg_two >();
			m_mbox->deliver_message( new msg_four() );
			m_mbox->deliver_signal< msg_five >();
		}

		void
		evt_default_one( const so_5::rt::event_data_t< msg_one > & )
		{
			m_sequence += "d1:";
		}

		void
		evt_default_two( const so_5::rt::event_data_t< msg_two > & )
		{
			m_sequence += "d2:";

			so_drop_subscription( m_mbox, &a_test_t::evt_default_one );

			m_mbox->deliver_signal< msg_one >();
			m_mbox->deliver_signal< msg_three >();
		}

		void
		evt_default_three( const so_5::rt::event_data_t< msg_three > & )
		{
			m_sequence += "d3:";

			so_change_state( st_1 );

			m_mbox->deliver_signal< msg_one >();
			m_mbox->deliver_signal< msg_two >();
		}

		void
		evt_default_four( const msg_four & )
		{
			m_sequence += "d4:";

			m_mbox->deliver_message( new msg_four() );

			so_drop_subscription( m_mbox, &a_test_t::evt_default_four );
			so_drop_subscription( m_mbox, st_1, &a_test_t::evt_default_four );
		}

		void
		evt_five()
		{
			m_sequence += "d5:";

			m_mbox->deliver_signal< msg_five >();

			so_drop_subscription_for_all_states( m_mbox,
					so_5::signal< msg_five > );
		}

		void
		evt_st_1_one( const so_5::rt::event_data_t< msg_one > & )
		{
			m_sequence += "1_d1:";
		}

		void
		evt_st_1_two( const so_5::rt::event_data_t< msg_two > & )
		{
			m_sequence += "1_d2:";

			so_drop_subscription( m_mbox, st_1, &a_test_t::evt_st_1_one );

			m_mbox->deliver_signal< msg_one >();
			m_mbox->deliver_signal< msg_three >();
		}

		void
		evt_st_1_three( const so_5::rt::event_data_t< msg_three > & )
		{
			m_sequence += "1_d3:";

			so_drop_subscription_for_all_states( m_mbox,
					&a_test_t::evt_default_one );

			so_change_state( st_2 );

			m_mbox->deliver_signal< msg_one >();
			m_mbox->deliver_signal< msg_two >();
			m_mbox->deliver_signal< msg_three >();
		}

		void
		evt_st_1_four( const msg_four & )
		{
			m_sequence += "1_d4:";
		}

		void
		evt_st_2_one( const so_5::rt::event_data_t< msg_one > & )
		{
			m_sequence += "2_d1:";
		}

		void
		evt_st_2_two( const so_5::rt::event_data_t< msg_two > & )
		{
			m_sequence += "2_d2:";
		}

		void
		evt_st_2_three( const so_5::rt::event_data_t< msg_three > & )
		{
			m_sequence += "2_d3:";

			so_environment().stop();
		}

	private :
		std::string & m_sequence;

		so_5::rt::mbox_ref_t m_mbox;

		so_5::rt::state_t st_1 = so_make_state();
		so_5::rt::state_t st_2 = so_make_state();
};

class test_env_t
{
	public :
		void
		init( so_5::rt::environment_t & env )
		{
			env.register_agent_as_coop(
					"test", new a_test_t( env, m_sequence ) );
		}

		void
		check_result() const
		{
			const std::string expected =
					"d1:d2:d4:d5:d3:1_d1:1_d2:1_d3:2_d2:2_d3:";

			if( m_sequence != expected )
				throw std::runtime_error( "Wrong message sequence: actial: " +
						m_sequence + ", expected: " + expected );
		}

	private :
		std::string m_sequence;
};

int
main( int argc, char * argv[] )
{
	try
	{
		test_env_t test_env;
		so_5::launch(
			[&]( so_5::rt::environment_t & env )
			{
				test_env.init( env );
			} );

		test_env.check_result();
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

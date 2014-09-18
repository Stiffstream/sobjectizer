/*
 * A test for coop reg/dereg notifications.
 */

#include <iostream>
#include <sstream>

#include <so_5/rt/h/rt.hpp>
#include <so_5/api/h/api.hpp>

struct msg_child_deregistered : public so_5::rt::signal_t {};

class a_child_t : public so_5::rt::agent_t
{
		typedef so_5::rt::agent_t base_type_t;

	public :
		a_child_t(
			so_5::rt::so_environment_t & env )
			:	base_type_t( env )
		{
		}

		void
		so_evt_start()
		{
			so_environment().deregister_coop(
					so_coop_name(),
					so_5::rt::dereg_reason::normal );
		}
};

class a_test_t : public so_5::rt::agent_t
{
	typedef so_5::rt::agent_t base_type_t;

	public :
		a_test_t(
			so_5::rt::so_environment_t & env,
			const so_5::rt::coop_reg_notificator_t & reg_notificator,
			const so_5::rt::coop_dereg_notificator_t & dereg_notificator )
			:	base_type_t( env )
			,	m_reg_notificator( reg_notificator )
			,	m_dereg_notificator( dereg_notificator )
			,	m_mbox( env.create_local_mbox() )
		{}

		void
		so_define_agent()
		{
			so_subscribe( m_mbox ).event( &a_test_t::evt_child_deregistered );
		}

		void
		so_evt_start()
		{
			auto child_coop = so_environment().create_coop( "child" );
			child_coop->set_parent_coop_name( so_coop_name() );
			child_coop->add_reg_notificator( m_reg_notificator );
			child_coop->add_dereg_notificator( m_dereg_notificator );
			child_coop->add_dereg_notificator(
					[this]( so_5::rt::so_environment_t &,
						const std::string &,
						const so_5::rt::coop_dereg_reason_t &)
					{
						m_mbox->deliver_signal< msg_child_deregistered >();
					} );

			child_coop->add_agent( new a_child_t( so_environment() ) );

			so_environment().register_coop( std::move( child_coop ) );
		}

		void
		evt_child_deregistered(
			const so_5::rt::event_data_t< msg_child_deregistered > & )
		{
			so_environment().stop();
		}

	private :
		const so_5::rt::coop_reg_notificator_t m_reg_notificator;
		const so_5::rt::coop_dereg_notificator_t m_dereg_notificator;

		const so_5::rt::mbox_ref_t m_mbox;
};

class test_env_t
{
	public :
		test_env_t()
			:	m_reg_notify_received( false )
			,	m_dereg_notify_received( false )
		{}

		void
		init( so_5::rt::so_environment_t & env )
		{
			auto on_reg =
					[this]( so_5::rt::so_environment_t &,
							const std::string & )
					{
						m_reg_notify_received = true;
					};
			auto on_dereg =
					[this]( so_5::rt::so_environment_t &,
							const std::string &,
							const so_5::rt::coop_dereg_reason_t &)
					{
						m_dereg_notify_received = true;
					};

			env.register_agent_as_coop(
					"test",
					new a_test_t( env, on_reg, on_dereg ) );
		}

		void
		check_result() const
		{
			if( !m_reg_notify_received )
				throw std::runtime_error( "registration notify not received" );
			if( !m_dereg_notify_received )
				throw std::runtime_error( "deregistration notify not received" );
		}

	private :
		bool m_reg_notify_received;
		bool m_dereg_notify_received;
};

int
main( int argc, char * argv[] )
{
	try
	{
		test_env_t test_env;
		so_5::api::run_so_environment_on_object(
				test_env,
				&test_env_t::init,
				so_5::rt::so_environment_params_t() );

		test_env.check_result();
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


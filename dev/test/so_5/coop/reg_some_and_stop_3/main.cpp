/*
 * A test for register cooperation from another cooperation.
*/

#include <iostream>
#include <map>
#include <exception>
#include <stdexcept>

#include <so_5/h/types.hpp>

#include <so_5/rt/h/rt.hpp>
#include <so_5/api/h/api.hpp>

so_5::rt::nonempty_name_t g_test_mbox_name( "test_mbox" );

struct slave_coop_finished_signal : public so_5::rt::signal_t {};

class a_slave_t
	:
		public so_5::rt::agent_t
{
		typedef so_5::rt::agent_t base_type_t;

	public:
		a_slave_t(
			so_5::rt::so_environment_t & env,
			const so_5::rt::mbox_ref_t & master_mbox )
			:
				base_type_t( env ),
				m_master_mbox( master_mbox )
		{}

		virtual ~a_slave_t()
		{}

		virtual void
		so_evt_start()
		{
			so_environment().deregister_coop(
				so_coop_name(),
				so_5::rt::dereg_reason::normal );

			m_master_mbox->deliver_signal< slave_coop_finished_signal >();
		}

	private:
		so_5::rt::mbox_ref_t m_master_mbox;

};

class a_master_t
	:
		public so_5::rt::agent_t
{
		typedef so_5::rt::agent_t base_type_t;

	public:
		a_master_t(
			so_5::rt::so_environment_t & env )
			:
				base_type_t( env )
		{}

		virtual ~a_master_t()
		{}

		virtual void
		so_evt_start()
		{
			so_5::rt::mbox_ref_t mbox = so_environment()
				.create_local_mbox( so_5::rt::nonempty_name_t(
					so_coop_name() + "_mbox" ) );

			so_subscribe( mbox )
				.in( so_default_state() )
					.event( &a_master_t::evt_slave_finished );

			so_environment().register_agent_as_coop(
					so_coop_name() + "_slave",
					new a_slave_t( so_environment(),  mbox ) );
		}

		void
		evt_slave_finished(
			const so_5::rt::event_data_t< slave_coop_finished_signal > & )
		{
			std::cout << "Shutdown\n";
			so_environment().stop();
		}
};

void
init( so_5::rt::so_environment_t & env )
{
	env.register_agent_as_coop( "test_coop_1", new a_master_t( env ) );
}

int
main( int argc, char * argv[] )
{
	try
	{
		so_5::api::run_so_environment(
			&init,
			std::move(
				so_5::rt::so_environment_params_t()
					.mbox_mutex_pool_size( 4 )
					.agent_event_queue_mutex_pool_size( 4 ) ) );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

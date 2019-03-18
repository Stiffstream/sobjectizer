/*
 * A test for register cooperation from another cooperation.
*/

#include <iostream>
#include <map>
#include <exception>
#include <stdexcept>

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

so_5::nonempty_name_t g_test_mbox_name( "test_mbox" );

struct slave_coop_finished_signal : public so_5::signal_t {};

class a_slave_t
	:
		public so_5::agent_t
{
		typedef so_5::agent_t base_type_t;

	public:
		a_slave_t(
			so_5::environment_t & env,
			so_5::mbox_t master_mbox )
			:
				base_type_t( env ),
				m_master_mbox( std::move(master_mbox) )
		{}

		void
		so_evt_start() override
		{
			so_deregister_agent_coop_normally();

			so_5::send< slave_coop_finished_signal >( m_master_mbox );
		}

	private:
		const so_5::mbox_t m_master_mbox;

};

class a_master_t
	:
		public so_5::agent_t
{
		typedef so_5::agent_t base_type_t;

	public:
		a_master_t(
			so_5::environment_t & env )
			:
				base_type_t( env )
		{}

		void
		so_evt_start() override
		{
			so_subscribe_self()
					.event( &a_master_t::evt_slave_finished );

			so_environment().register_agent_as_coop(
					so_environment().make_agent< a_slave_t >( so_direct_mbox() ) );
		}

		void
		evt_slave_finished(
			mhood_t< slave_coop_finished_signal > )
		{
			std::cout << "Shutdown\n";
			so_environment().stop();
		}
};

void
init( so_5::environment_t & env )
{
	env.register_agent_as_coop( env.make_agent< a_master_t >() );
}

int
main()
{
	run_with_time_limit( [] {
			so_5::launch( &init );
		},
		10 );

	return 0;
}

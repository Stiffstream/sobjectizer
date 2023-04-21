/*
 * A unit-test for testing deregistration of empty coop.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

void
init( so_5::environment_t & env )
{
	class a_supervisor_t final : public so_5::agent_t
	{
		so_5::coop_handle_t m_child;

	public :
		using so_5::agent_t::agent_t;

		void so_define_agent() override
		{
			so_subscribe_self()
				.event( [this](mhood_t<so_5::msg_coop_registered>) {
						so_environment().deregister_coop(
								m_child,
								so_5::dereg_reason::normal );
					} )
				.event( [this](mhood_t<so_5::msg_coop_deregistered>) {
						so_deregister_agent_coop_normally();
					} );
		}

		void so_evt_start() override
		{
			so_5::introduce_child_coop( *this,
					[this]( so_5::coop_t & coop ) {
						m_child = coop.handle();

						coop.add_reg_notificator(
								so_5::make_coop_reg_notificator( so_direct_mbox() ) );
						coop.add_dereg_notificator(
								so_5::make_coop_dereg_notificator( so_direct_mbox() ) );
					} );
		}
	};

	// Supervison coop.
	env.introduce_coop( []( so_5::coop_t & parent ) {
			parent.make_agent< a_supervisor_t >();
		} );
}

int
main()
{
	try
	{
		run_with_time_limit( []{ so_5::launch( init ); },
				20,
				"dereg_empty_coop" );

		return 0;
	}
	catch( const std::exception & x )
	{
		std::cerr << "Exception: " << x.what() << std::endl;
	}

	return 2;
}


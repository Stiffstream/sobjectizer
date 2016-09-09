/*
 * A simple test for getting stats about work thread activity.
 */

#include <iostream>
#include <map>
#include <exception>
#include <stdexcept>
#include <cstdlib>
#include <thread>
#include <chrono>

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

class a_test_t : public so_5::agent_t
	{
	public :
		a_test_t( context_t ctx )
			:	so_5::agent_t( ctx )
			{}

		virtual void
		so_define_agent() override
			{
				so_default_state().event(
						so_environment().stats_controller().mbox(),
						&a_test_t::evt_monitor_activity );
			}

		virtual void
		so_evt_start() override
			{
				create_child_coops();

				so_environment().stats_controller().turn_on();
			}

	private :
		unsigned int m_actual_values = { 0 };

		void
		evt_monitor_activity(
			const so_5::stats::messages::work_thread_activity & evt )
			{
				namespace stats = so_5::stats;

				std::cout << evt.m_prefix << evt.m_suffix
						<< " [" << evt.m_thread_id << "] ->\n"
						<< "  working: " << evt.m_stats.m_working_stats << "\n"
						<< "  waiting: " << evt.m_stats.m_waiting_stats << std::endl;

				++m_actual_values;

				if( 12 == m_actual_values )
					so_deregister_agent_coop_normally();
			}

		void
		create_child_coops()
			{
				for( int i = 0; i != 10; ++i )
					{
						auto coop = so_5::create_child_coop(
								*this, so_5::autoname );
						coop->define_agent();

						so_environment().register_coop( std::move( coop ) );
					}
			}
	};

void
init( so_5::environment_t & env )
	{
		env.introduce_coop( []( so_5::coop_t & coop ) {
			coop.make_agent< a_test_t >();

			struct next : public so_5::signal_t {};
			auto agent = coop.define_agent(
				so_5::disp::one_thread::create_disp_binder( "busy" ) );
			agent
				.on_start( [agent]{ so_5::send< next >( agent ); } )
				.event< next >( agent, [agent]{
					so_5::send< next >( agent );
					std::this_thread::sleep_for( std::chrono::seconds(3) );
				} );
		} );
	}

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				so_5::launch( &init,
					[]( so_5::environment_params_t & params ) {
						namespace otd = so_5::disp::one_thread;

						params.turn_work_thread_activity_tracking_on();
						params.add_named_dispatcher( "empty", otd::create_disp() );
						params.add_named_dispatcher( "busy", otd::create_disp() );
					} );
			},
			20,
			"simple coop count monitoring test" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


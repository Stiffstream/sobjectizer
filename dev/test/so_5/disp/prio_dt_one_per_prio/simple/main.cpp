/*
 * A simple test for prio_dedicated_threads::one_per_prio dispatcher.
 */

#include <so_5/all.hpp>

#include <various_helpers_1/time_limited_execution.hpp>

struct msg_hello : public so_5::signal_t {};

class a_test_t : public so_5::agent_t
{
	public:
		a_test_t( context_t ctx )
			:	so_5::agent_t( ctx + so_5::prio::p7 )
		{}

		virtual void
		so_define_agent() override
		{
			so_subscribe_self().event< msg_hello >( &a_test_t::evt_hello );
		}

		virtual void
		so_evt_start() override
		{
			so_direct_mbox()->deliver_signal< msg_hello >();
		}

		void
		evt_hello()
		{
			so_environment().stop();
		}
};

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				static_assert( 8 == so_5::prio::total_priorities_count,
						"total_priorities_count must be 8" );

				using namespace so_5::disp::prio_dedicated_threads::one_per_prio;

				so_5::launch(
					[]( so_5::environment_t & env )
					{
						env.register_agent_as_coop(
								"test",
								new a_test_t( env ),
								create_disp_binder( "prio_dispatcher" ) );
					},
					[]( so_5::environment_params_t & params )
					{
						params.add_named_dispatcher(
								"prio_dispatcher",
								create_disp() );
					} );
			},
			20,
			"simple prio_dedicated_threads::one_per_prio dispatcher test" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


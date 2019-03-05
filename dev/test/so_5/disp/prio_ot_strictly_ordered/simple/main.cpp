/*
 * A simple test for prio_one_thread::strictly_ordered dispatcher.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

struct msg_hello : public so_5::signal_t {};

class a_test_t : public so_5::agent_t
{
	public:
		a_test_t( context_t ctx )
			:	so_5::agent_t( ctx + so_5::prio::p7 )
		{}

		void
		so_define_agent() override
		{
			so_subscribe_self().event( &a_test_t::evt_hello );
		}

		void
		so_evt_start() override
		{
			so_5::send< msg_hello >( *this );
		}

		void
		evt_hello(mhood_t< msg_hello >)
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

				so_5::launch(
					[]( so_5::environment_t & env )
					{
						using namespace so_5::disp::prio_one_thread::strictly_ordered;

						env.register_agent_as_coop(
								"test",
								env.make_agent< a_test_t >(),
								make_dispatcher( env ).binder() );
					} );
			},
			20 );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


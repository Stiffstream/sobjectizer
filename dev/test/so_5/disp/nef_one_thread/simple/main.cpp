/*
 * A simple test for nef_one_thread dispatcher.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

struct msg_hello : public so_5::signal_t {};

class a_test_t : public so_5::agent_t
{
	public:
		a_test_t( context_t ctx )
			:	so_5::agent_t{ std::move(ctx) }
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
				using namespace so_5::disp::nef_one_thread;

				so_5::launch(
					[]( so_5::environment_t & env )
					{
						env.register_agent_as_coop(
								env.make_agent< a_test_t >(),
								make_dispatcher( env ).binder() );
					} );
			},
			5 );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


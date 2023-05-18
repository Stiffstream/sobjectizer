/*
 * Test for a normal shutdown after std::bad_alloc.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

#include "../../custom_new_operator.hpp"

struct msg_hello final : public so_5::signal_t {};
struct msg_let_crash final : public so_5::signal_t {};

class a_test_t final : public so_5::agent_t
{
	public:
		a_test_t( context_t ctx )
			:	so_5::agent_t{ std::move(ctx) }
		{}

		void
		so_define_agent() override
		{
			so_subscribe_self().event( &a_test_t::evt_hello );
			so_subscribe_self().event( &a_test_t::evt_let_crash );
		}

		void
		so_evt_start() override
		{
			so_5::send< msg_hello >( *this );
		}

		void
		evt_hello(mhood_t< msg_hello >)
		{
			so_5::test::disp::custom_new::turn_should_throw_on();
			std::puts( "should_throw is turned on" );
			so_5::send< msg_let_crash >( *this );
		}

		void
		evt_let_crash(mhood_t<msg_let_crash>)
		{
			// We shouldn't received this message!
			std::cerr << "evt_let_crash shouldn't be called!" << std::endl;
			std::abort();
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
				using namespace so_5::disp::prio_one_thread::strictly_ordered;

				so_5::launch(
					[]( so_5::environment_t & env )
					{
						env.register_agent_as_coop(
								env.make_agent< a_test_t >(),
								make_dispatcher( env ).binder() );
					},
					[]( so_5::environment_params_t & params ) {
						params.exception_reaction(
								so_5::exception_reaction_t::shutdown_sobjectizer_on_exception );
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


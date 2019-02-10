/*
 * A test of deregistration of cooperation when demand queue for
 * dispatcher is not empty.
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

struct send_next : public so_5::signal_t {};
struct stop : public so_5::signal_t {};

void
fill_coop(
	so_5::coop_t & coop )
	{
		using namespace so_5;

		class a_test_t final : public agent_t
		{
		public :
			a_test_t( context_t ctx )
				:	agent_t{
						ctx +
						agent_t::limit_then_drop< send_next >( 100 ) +
						agent_t::limit_then_drop< stop >( 1 ) +
						prio::p0 }
			{
				so_subscribe_self()
					.event( [this](mhood_t<send_next>) {
							send< send_next >( *this );
							send< send_next >( *this );
						} )
					.event( [this](mhood_t<stop>) {
							so_deregister_agent_coop_normally();
						} );
			}

			void so_evt_start() override
			{
				send< send_next >( *this );
				send_delayed< stop >( *this, std::chrono::milliseconds( 350 ) );
			}
		};

		coop.make_agent< a_test_t >();
	}

int
main()
{
	try
	{
		run_with_time_limit(
			[]()
			{
				so_5::launch(
					[]( so_5::environment_t & env )
					{
						using namespace so_5::disp::prio_one_thread::strictly_ordered;

						env.introduce_coop(
								create_private_disp( env )->binder(),
								fill_coop );
					} );
			},
			20,
			"deregistration of coop on prio_one_thread::strictly_ordered dispatcher test" );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


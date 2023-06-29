/*
 * A simple test for circular message subscription.
 */

#include <iostream>
#include <sstream>

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

namespace test {

struct msg_data final : public so_5::message_t
	{
		int m_v;

		msg_data( int v ) : m_v{ v } {}
	};

struct msg_stop final : public so_5::signal_t {};

class a_test_t final : public so_5::agent_t
	{
	public:
		a_test_t( context_t ctx )
			:	so_5::agent_t{ std::move(ctx) }
			{}

		void
		so_define_agent() override
			{
				so_subscribe_self()
					.event( &a_test_t::evt_stop )
					;
			}

		void
		so_evt_start() override
			{
				auto m1 = so_environment().create_mbox();
				auto m2 = so_environment().create_mbox();

				so_5::single_sink_binding_t m1_binding;
				so_5::single_sink_binding_t m2_binding;

				m1_binding.bind< msg_data >( m1, so_5::wrap_to_msink( m2 ) );
				m2_binding.bind< msg_data >( m2, so_5::wrap_to_msink( m1 ) );

				// This message has to be ignored.
				so_5::send< msg_data >( m1, 2 );

				so_5::send< msg_stop >( *this );
			}

		void
		evt_stop( mhood_t< msg_stop > )
			{
				so_deregister_agent_coop_normally();
			}
	};

void
introduce_test_coop( so_5::environment_t & env )
	{
		env.introduce_coop( []( so_5::coop_t & coop ) {
				coop.make_agent< a_test_t >();
			} );
	}

} /* namespace test */

int
main()
	{
		run_with_time_limit(
			[]()
			{
				so_5::launch(
					[]( so_5::environment_t & env )
					{
						test::introduce_test_coop( env );
					},
					[]( so_5::environment_params_t & params )
					{
						params.message_delivery_tracer(
								so_5::msg_tracing::std_cout_tracer() );
					} );
			},
			5 );

		return 0;
	}


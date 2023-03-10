/*
 * A simple case for single_sink_binding_t::clear.
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
		int m_messages_received{};

	public:
		a_test_t( context_t ctx )
			:	so_5::agent_t{ std::move(ctx) }
			{}

		void
		so_define_agent() override
			{
				so_subscribe_self()
					.event( &a_test_t::evt_data )
					.event( &a_test_t::evt_stop )
					;
			}

		void
		so_evt_start() override
			{
				auto dest = so_environment().create_mbox();

				so_5::single_sink_binding_t msg_data_binding;
				so_5::single_sink_binding_t msg_stop_binding;

				ensure_or_die( msg_data_binding.empty(),
						"(1) msg_data_binding has to be empty" );
				ensure_or_die( msg_stop_binding.empty(),
						"(2) msg_data_binding has to be empty" );

				msg_data_binding.bind< msg_data >( dest,
						so_5::wrap_to_msink( so_direct_mbox() ) );
				ensure_or_die( not msg_data_binding.empty(),
						"(3) msg_data_binding should have a value" );
				ensure_or_die( msg_data_binding.has_value(),
						"(4) msg_data_binding should have a value" );

				msg_stop_binding.bind< msg_stop >( dest,
						so_5::wrap_to_msink( so_direct_mbox() ) );
				ensure_or_die( not msg_stop_binding.empty(),
						"(5) msg_stop_binding should have a value" );
				ensure_or_die( msg_stop_binding.has_value(),
						"(6) msg_stop_binding should have a value" );

				so_5::send< msg_data >( dest, 1 );

				msg_data_binding.clear();
				ensure_or_die( msg_data_binding.empty(),
						"(7) msg_data_binding has to be empty" );

				so_5::send< msg_data >( dest, 2 );

				so_5::send< msg_stop >( dest );
			}

		void
		evt_data( mhood_t< msg_data > )
			{
				++m_messages_received;
			}

		void
		evt_stop( mhood_t< msg_stop > )
			{
				ensure_or_die( 1 == m_messages_received,
						"unexpected value of m_messages_received: " +
						std::to_string( m_messages_received ) );
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
					} );
			},
			5 );

		return 0;
	}


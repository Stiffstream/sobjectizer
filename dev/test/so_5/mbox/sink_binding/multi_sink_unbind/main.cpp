/*
 * A simple case for single_sink_binding_t.
 */

#include <iostream>
#include <sstream>

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>
#include <test/3rd_party/various_helpers/ensure.hpp>

namespace test {

struct msg_data final : public so_5::message_t
	{
		int m_i;
		int m_v;

		msg_data( int i, int v ) : m_i{ i }, m_v{ v } {}
	};

[[nodiscard]]
std::string
to_string( const msg_data & msg )
	{
		return std::to_string( msg.m_i ) + ":" + std::to_string( msg.m_v );
	}

struct msg_signal final : public so_5::signal_t {};

class a_test_t final : public so_5::agent_t
	{
		so_5::multi_sink_binding_t<> m_binding;

		const so_5::mbox_t m_d1;
		const so_5::mbox_t m_d2;
		const so_5::mbox_t m_d3;
		const so_5::mbox_t m_signal_dest;

		std::string & m_protocol;
	
	public:
		a_test_t( context_t ctx, std::string & protocol )
			:	so_5::agent_t{ std::move(ctx) }
			,	m_d1{ so_environment().create_mbox() }
			,	m_d2{ so_environment().create_mbox() }
			,	m_d3{ so_environment().create_mbox() }
			,	m_signal_dest{ so_environment().create_mbox() }
			,	m_protocol{ protocol }
			{}

		void
		so_define_agent() override
			{
				so_subscribe_self()
					.event( &a_test_t::evt_data )
					.event( &a_test_t::evt_signal )
					;
			}

		void
		so_evt_start() override
			{
				auto self_msink = so_5::wrap_to_msink( so_direct_mbox() );

				m_binding.bind< msg_signal >( m_signal_dest, self_msink );

				m_binding.bind< msg_data >( m_d1, self_msink );
				m_binding.bind< msg_data >( m_d2, self_msink );
				m_binding.bind< msg_data >( m_d3, self_msink );

				so_5::send< msg_data >( m_d1, 1, 0 );
				so_5::send< msg_data >( m_d2, 2, 0 );
				so_5::send< msg_data >( m_d3, 3, 0 );

				m_binding.unbind< msg_data >( m_d2, self_msink );

				so_5::send< msg_data >( m_d1, 1, 1 );
				so_5::send< msg_data >( m_d2, 2, 1 );
				so_5::send< msg_data >( m_d3, 3, 1 );

				m_binding.unbind_all_for( m_d3, self_msink );

				so_5::send< msg_data >( m_d1, 1, 2 );
				so_5::send< msg_data >( m_d2, 2, 2 );
				so_5::send< msg_data >( m_d3, 3, 2 );

				m_binding.bind< msg_data >( m_d2, self_msink );
				m_binding.unbind< msg_data >( m_d1, self_msink );

				so_5::send< msg_data >( m_d1, 1, 3 );
				so_5::send< msg_data >( m_d2, 2, 3 );
				so_5::send< msg_data >( m_d3, 3, 3 );

				so_5::send< msg_signal >( m_signal_dest );
			}

	private:
		void
		evt_data( mhood_t<msg_data> cmd )
			{
				m_protocol += to_string( *cmd ) + ";";
			}

		void
		evt_signal( mhood_t<msg_signal> )
			{
				so_deregister_agent_coop_normally();
			}
	};

void
introduce_test_coop(
	so_5::environment_t & env,
	std::string & protocol )
	{
		env.introduce_coop( [&protocol]( so_5::coop_t & coop ) {
				coop.make_agent< a_test_t >( protocol );
			} );
	}

} /* namespace test */

int
main()
	{
		run_with_time_limit(
			[]()
			{
				std::string protocol;
				so_5::launch(
					[&protocol]( so_5::environment_t & env )
					{
						test::introduce_test_coop( env, protocol );
					} );

				const std::string expected{
						"1:0;2:0;3:0;"
						"1:1;3:1;"
						"1:2;"
						"2:3;"
					};
				ensure_or_die( expected == protocol,
						std::string( "invalid result protocol: '" ) + protocol +
						"', expected: '" + expected + "'" );
			},
			5 );

		return 0;
	}


/*
 * A simple case for single_sink_binding_t and delivery_filter.
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

struct msg_signal final : public so_5::signal_t {};

class a_producer_t final : public so_5::agent_t
	{
		const so_5::mbox_t m_dest;
	
	public:
		a_producer_t( context_t ctx )
			:	so_5::agent_t{ std::move(ctx) }
			,	m_dest{ so_environment().create_mbox() }
			{}

		[[nodiscard]]
		so_5::mbox_t
		dest() const { return m_dest; }

		void
		so_evt_start() override
			{
				so_5::send< msg_data >( m_dest, 1 );
				so_5::send< msg_data >( m_dest, 10 );
				so_5::send< msg_data >( m_dest, 9 );
				so_5::send< msg_data >( m_dest, 11 );
				so_5::send< msg_signal >( m_dest );
			}
	};

class a_consumer_t final : public so_5::agent_t
	{
		int m_messages_received{};

	public:
		using so_5::agent_t::agent_t;

		void
		so_define_agent() override
			{
				so_subscribe_self()
					.event( &a_consumer_t::evt_data )
					.event( &a_consumer_t::evt_signal )
					;
			}

	private:
		void
		evt_data( mhood_t<msg_data> cmd )
			{
				std::cout << "data: " << cmd->m_v << std::endl;
				++m_messages_received;
			}

		void
		evt_signal( mhood_t<msg_signal> )
			{
				ensure_or_die( 2 == m_messages_received,
						"unexpected number of messages_received: "
						+ std::to_string( m_messages_received ) );
				so_deregister_agent_coop_normally();
			}
	};

void
introduce_test_coop( so_5::environment_t & env )
	{
		env.introduce_coop( []( so_5::coop_t & coop ) {
				auto * producer = coop.make_agent< a_producer_t >();
				auto * consumer = coop.make_agent< a_consumer_t >();

				coop.take_under_control(
						std::make_unique< so_5::single_sink_binding_t >() )->
					bind< msg_data >(
							producer->dest(),
							so_5::wrap_to_msink( consumer->so_direct_mbox() ),
							[]( const msg_data & msg ) {
								return msg.m_v < 10;
							} );

				coop.take_under_control(
						std::make_unique< so_5::single_sink_binding_t >() )->
					bind< msg_signal >(
							producer->dest(),
							so_5::wrap_to_msink( consumer->so_direct_mbox() ) );
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


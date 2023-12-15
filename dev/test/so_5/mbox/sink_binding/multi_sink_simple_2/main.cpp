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
		int m_v;

		msg_data( int v ) : m_v{ v } {}
	};

struct msg_signal final : public so_5::signal_t {};

class a_producer_t final : public so_5::agent_t
	{
		const so_5::mbox_t m_data_dest;
		const so_5::mbox_t m_signal_dest;
	
	public:
		a_producer_t( context_t ctx )
			:	so_5::agent_t{ std::move(ctx) }
			,	m_data_dest{ so_environment().create_mbox() }
			,	m_signal_dest{ so_environment().create_mbox() }
			{}

		[[nodiscard]]
		so_5::mbox_t
		data_dest() const { return m_data_dest; }

		[[nodiscard]]
		so_5::mbox_t
		signal_dest() const { return m_signal_dest; }

		void
		so_evt_start() override
			{
				so_5::send< msg_data >( m_data_dest, 1 );
				so_5::send< msg_signal >( m_signal_dest );
			}
	};

class a_collector_t final : public so_5::agent_t
	{
		const unsigned int m_expected_acks;
		unsigned int m_received_acks{};

	public:
		struct msg_ack final : public so_5::signal_t {};

		a_collector_t( context_t ctx, unsigned int expected_acks )
			:	so_5::agent_t{ std::move(ctx) }
			,	m_expected_acks{ expected_acks }
			{}

		void
		so_define_agent() override
			{
				so_subscribe_self().event( [this]( mhood_t<msg_ack> ) {
						++m_received_acks;
						if( m_received_acks == m_expected_acks )
							so_deregister_agent_coop_normally();
					} );
			}
	};

class a_consumer_t final : public so_5::agent_t
	{
		state_t st_wait_data{ this, "wait_data" };
		state_t st_wait_signal{ this, "wait_signal" };

		const so_5::mbox_t m_collector_mbox;

	public:
		a_consumer_t( context_t ctx, so_5::mbox_t collector_mbox )
			:	so_5::agent_t{ std::move(ctx) }
			,	m_collector_mbox{ std::move(collector_mbox) }
			{}

		void
		so_define_agent() override
			{
				st_wait_data.activate();

				st_wait_data
					.event( &a_consumer_t::evt_data )
					;

				st_wait_signal
					.event( &a_consumer_t::evt_signal )
					;
			}

	private:
		void
		evt_data( mhood_t<msg_data> cmd )
			{
				std::cout << "data: " << cmd->m_v << std::endl;
				st_wait_signal.activate();
			}

		void
		evt_signal( mhood_t<msg_signal> )
			{
				so_5::send< a_collector_t::msg_ack >( m_collector_mbox );
			}
	};

void
introduce_test_coop( so_5::environment_t & env )
	{
		env.introduce_coop( []( so_5::coop_t & coop ) {
				auto collector_mbox = coop.make_agent< a_collector_t >( 2u )->so_direct_mbox();

				auto * producer = coop.make_agent< a_producer_t >();
				auto * consumer_1 = coop.make_agent< a_consumer_t >( collector_mbox );
				auto * consumer_2 = coop.make_agent< a_consumer_t >( collector_mbox );

				auto consumer_1_msink = so_5::wrap_to_msink(
						consumer_1->so_direct_mbox() );
				auto consumer_2_msink = so_5::wrap_to_msink(
						consumer_2->so_direct_mbox() );

				auto * binding = coop.take_under_control(
						std::make_unique< so_5::multi_sink_binding_t<> >() );

				binding->bind< msg_data >(
						producer->data_dest(),
						consumer_1_msink );
				binding->bind< msg_data >(
						producer->data_dest(),
						consumer_2_msink );

				binding->bind< msg_signal >(
						producer->signal_dest(),
						consumer_1_msink );
				binding->bind< msg_signal >(
						producer->signal_dest(),
						consumer_2_msink );
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


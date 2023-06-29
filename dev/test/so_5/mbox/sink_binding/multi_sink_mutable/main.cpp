/*
 * A simple case for multi_sink_binding_t and mutable msg.
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
		a_producer_t( context_t ctx, so_5::mbox_t dest )
			:	so_5::agent_t{ std::move(ctx) }
			,	m_dest{ dest }
			{}

		void
		so_evt_start() override
			{
				so_5::send< so_5::mutable_msg<msg_data> >( m_dest, 1 );
				so_5::send< msg_signal >( m_dest );
			}
	};

class a_consumer_t final : public so_5::agent_t
	{
		state_t st_wait_data{ this, "wait_data" };
		state_t st_wait_signal{ this, "wait_signal" };

	public:
		using so_5::agent_t::agent_t;

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
		evt_data( mutable_mhood_t<msg_data> cmd )
			{
				std::cout << "data: " << cmd->m_v << std::endl;
				st_wait_signal.activate();
			}

		void
		evt_signal( mhood_t<msg_signal> )
			{
				so_deregister_agent_coop_normally();
			}
	};

void
introduce_test_coop( so_5::environment_t & env )
	{
		env.introduce_coop( []( so_5::coop_t & coop ) {
				auto data_mbox = so_5::make_unique_subscribers_mbox( coop.environment() );

				coop.make_agent< a_producer_t >( data_mbox );
				auto * consumer = coop.make_agent< a_consumer_t >();
				auto consumer_msink = so_5::wrap_to_msink( consumer->so_direct_mbox() );

				auto * binding = coop.take_under_control(
						std::make_unique< so_5::multi_sink_binding_t<> >() );
				
				binding->bind< so_5::mutable_msg<msg_data> >( data_mbox, consumer_msink );
				binding->bind< msg_signal >( data_mbox, consumer_msink );
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


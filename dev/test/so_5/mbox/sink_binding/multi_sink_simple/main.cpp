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
		evt_data( mhood_t<msg_data> cmd )
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
				auto * producer = coop.make_agent< a_producer_t >();
				auto * consumer = coop.make_agent< a_consumer_t >();

				auto consumer_msink = so_5::wrap_to_msink(
						consumer->so_direct_mbox() );

				auto * binding = coop.take_under_control(
						std::make_unique< so_5::multi_sink_binding_t<> >() );

				binding->bind< msg_data >(
						producer->data_dest(),
						consumer_msink );

				binding->bind< msg_signal >(
						producer->signal_dest(),
						consumer_msink );
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


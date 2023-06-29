/*
 * A sample of the simple case for single_sink_binding.
 */

#include <iostream>

// Main SObjectizer header files.
#include <so_5/all.hpp>

// Message that carries some data.
struct msg_data final : public so_5::message_t
{
	int m_value;

	msg_data( int v ) : m_value{ v } {}
};

// Agent that creates MPMC mbox and sends new msg_data to it periodically.
class data_generator final : public so_5::agent_t
{
		// Signal to be used for new data generation.
		struct msg_generate_next final : public so_5::signal_t {};

		// Mbox to be used for data distribution.
		const so_5::mbox_t m_dest;

		// Counter to be used for data generation.
		int m_value_counter{};

	public:
		data_generator( context_t ctx )
			:	so_5::agent_t{ std::move(ctx) }
			,	m_dest{ so_environment().create_mbox() }
		{}

		// Accessor for mbox used for data distribution.
		[[nodiscard]] so_5::mbox_t distribution_mbox() const
		{
			return m_dest;
		}

		void so_define_agent() override
		{
			so_subscribe_self().event( &data_generator::evt_generate_next );
		}

		void so_evt_start() override
		{
			so_5::send< msg_generate_next >( *this );
		}

	private:
		void evt_generate_next( mhood_t< msg_generate_next > )
		{
			// New message has to be sent to the destination mbox.
			so_5::send< msg_data >( m_dest, ++m_value_counter );

			// Next iteration after some delay.
			so_5::send_delayed< msg_generate_next >( *this,
					std::chrono::milliseconds{ 25 } );
		}
};

// Agent that waits msg_data messages to be sent to its direct_mbox.
class data_consumer final : public so_5::agent_t
{
		// Number of consumed messages.
		int m_messages_consumed{};

	public:
		data_consumer( context_t ctx )
			:	so_5::agent_t{ std::move(ctx) }
		{}

		void so_define_agent() override
		{
			so_subscribe_self()
				.event( &data_consumer::evt_data );
		}

	private:
		void evt_data( mhood_t< msg_data > cmd )
		{
			std::cout << "data_consumer: new data: " << cmd->m_value << std::endl;

			++m_messages_consumed;
			if( m_messages_consumed > 3 )
				// It's time to finish the demo.
				so_deregister_agent_coop_normally();
		}
};
int main()
{
	try
	{
		// Starting SObjectizer.
		so_5::launch(
			[]( so_5::environment_t & env )
			{
				// Create a new coop with generator and consumer agents.
				env.introduce_coop( []( so_5::coop_t & coop ) {
						auto * generator = coop.make_agent< data_generator >();
						auto * consumer = coop.make_agent< data_consumer >();

						// Make a binding for msg_data message.
						// Binding should live as long as consumer agent,
						// so place it under the control of the coop.
						auto * binding = coop.take_under_control(
								std::make_unique< so_5::single_sink_binding_t >() );
						// Message msg_data from generator.distribution_mbox
						// should go into consumer.direct_mbox.
						binding->bind< msg_data >(
								generator->distribution_mbox(),
								so_5::wrap_to_msink( consumer->so_direct_mbox() ) );
					} );
			} );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


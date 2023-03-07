/*
 * A sample of use custom direct mbox for an agent.
 */

#include <iostream>

// Main SObjectizer header files.
#include <so_5/all.hpp>

// Messages to be used for producer-consumer interaction.
struct msg_first final : public so_5::message_t
{
	std::string m_payload;

	msg_first( std::string payload ) : m_payload{ std::move(payload) }
	{}
};

struct msg_second final : public so_5::message_t
{
	std::string m_payload;

	msg_second( std::string payload ) : m_payload{ std::move(payload) }
	{}
};

// Consumer that receives and handles msg_first/msg_second.
// NOTE: it's a final class, we can't inherit from it and change the
// behavior in a derived class.
class a_consumer_t final : public so_5::agent_t
{
public:
	a_consumer_t( context_t ctx, std::string name )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_name{ std::move(name) }
	{}

	void so_define_agent() override
	{
		so_subscribe_self()
			.event( &a_consumer_t::evt_first )
			.event( &a_consumer_t::evt_second )
			;
	}

private:
	const std::string m_name;

	void evt_first( mutable_mhood_t<msg_first> cmd )
	{
		std::cout << m_name << " => msg_first arrived: "
				<< cmd->m_payload << std::endl;
	}

	void evt_second( mutable_mhood_t<msg_second> cmd )
	{
		std::cout << m_name << " => msg_second arrived: "
				<< cmd->m_payload << std::endl;
	}
};

// Producer that generates msg_first/msg_second messages and sends them
// to the direct mbox of consumer agent.
class a_producer_t final : public so_5::agent_t
{
	// Signal to be used to finish the work.
	struct msg_quit final : public so_5::signal_t {};

public:
	a_producer_t(
		context_t ctx,
		const a_consumer_t & consumer )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_dest_mbox{ consumer.so_direct_mbox() }
	{}

	void so_define_agent() override
	{
		so_subscribe_self().event( &a_producer_t::evt_quit );
	}

	void so_evt_start() override
	{
		so_5::send< so_5::mutable_msg<msg_first> >( m_dest_mbox, "Hello, " );
		so_5::send< so_5::mutable_msg<msg_second> >( m_dest_mbox, "World!" );

		so_5::send< msg_quit >( *this );
	}

private:
	const so_5::mbox_t m_dest_mbox;

	void evt_quit( mhood_t<msg_quit> )
	{
		std::cout << "--- Work completed ---" << std::endl;
		so_deregister_agent_coop_normally();
	}
};

// Runs SObjectizer and creates a coop with just two agents (producer and
// original consumer).
void run_normal_scenario()
{
	std::cout << "*** Start of normal scenario ***" << std::endl;
	so_5::launch( []( so_5::environment_t & env ) {
			env.introduce_coop( []( so_5::coop_t & coop ) {
					auto * consumer = coop.make_agent< a_consumer_t >( "normal" );
					coop.make_agent< a_producer_t >( *consumer );
				} );
		} );
}

// Agent that plays role of actual msg_second consumer.
class a_actual_consumer_t final : public so_5::agent_t
{
public:
	a_actual_consumer_t( context_t ctx, std::string name )
		:	so_5::agent_t{ std::move(ctx) }
		,	m_name{ std::move(name) }
	{}

	void so_define_agent() override
	{
		so_subscribe_self()
			.event( &a_actual_consumer_t::evt_second )
			;
	}

private:
	const std::string m_name;

	void evt_second( mutable_mhood_t<msg_second> cmd )
	{
		std::cout << m_name << " => msg_second arrived: "
				<< cmd->m_payload << std::endl;
	}
};

// Special mbox for intercepting msg_second message.
class intercepting_mbox_t final : public so_5::abstract_message_box_t
{
	// Source mbox to that all messages, except msg_second, should go.
	const so_5::mbox_t m_source;
	// Target mbox for msg_second messages.
	const so_5::mbox_t m_target;

	[[nodiscard]]
	static bool should_intercept( const std::type_index & msg_type )
	{
		// NOTE: msg_second is sent as mutable_msg, so we have to check
		// type_index for mutable_msg<msg_second>, not for just msg_second.
		return msg_type == typeid(so_5::mutable_msg<msg_second>);
	}

public:
	intercepting_mbox_t(
		so_5::mbox_t source,
		so_5::mbox_t target )
		:	m_source{ std::move(source) }
		,	m_target{ std::move(target) }
	{}

	so_5::mbox_id_t id() const override
	{
		// This mbox has no own ID.
		return m_source->id();
	}

	void subscribe_event_handler(
		const std::type_index & msg_type,
		so_5::abstract_message_sink_t & subscriber ) override
	{
		if( !should_intercept( msg_type ) )
			m_source->subscribe_event_handler(
					msg_type,
					subscriber );
	}

	void unsubscribe_event_handlers(
		const std::type_index & msg_type,
		so_5::abstract_message_sink_t & subscriber ) override
	{
		if( !should_intercept( msg_type ) )
			m_source->unsubscribe_event_handlers(
					msg_type,
					subscriber );
	}

	std::string query_name() const override
	{
		// This mbox has no own name.
		return m_source->query_name();
	}

	so_5::mbox_type_t type() const override
	{
		// This mbox has no own type.
		return m_target->type();
	}

	void do_deliver_message(
		so_5::message_delivery_mode_t delivery_mode,
		const std::type_index & msg_type,
		const so_5::message_ref_t & message,
		unsigned int redirection_deep ) override
	{
		auto & dest = should_intercept( msg_type ) ? *m_target : *m_source;

		// Some tracing just for demo purposes.
		std::cout << "do_deliver_message for '" << msg_type.name()
				<< "', should_intercept: " << should_intercept( msg_type )
				<< ", dest_id: " << dest.id() << std::endl;

		dest.do_deliver_message(
				delivery_mode,
				msg_type,
				message,
				redirection_deep );
	}

	void set_delivery_filter(
		const std::type_index & msg_type,
		const so_5::delivery_filter_t & filter,
		so_5::abstract_message_sink_t & subscriber ) override
	{
		if( !should_intercept( msg_type ) )
			m_source->set_delivery_filter(
					msg_type,
					filter,
					subscriber );
	}

	void drop_delivery_filter(
		const std::type_index & msg_type,
		so_5::abstract_message_sink_t & subscriber ) noexcept override
	{
		if( !should_intercept( msg_type ) )
			m_source->drop_delivery_filter(
					msg_type,
					subscriber );
	}

	so_5::environment_t & environment() const noexcept override
	{
		return m_source->environment();
	}
};

// Runs SObjectizer and creates a coop with three agents: producer,
// original consumer, and actual consumer for msg_second.
// An intercepting mbox is used to stole msg_second message from
// the original consumer and redirect it to the actual consumer.
void run_intercepting_scenario()
{
	std::cout << "*** Start of interception scenario ***" << std::endl;
	so_5::launch( []( so_5::environment_t & env ) {
			env.introduce_coop( []( so_5::coop_t & coop ) {
					auto * actual_consumer = coop.make_agent< a_actual_consumer_t >( "actual" );

					// A factory for creation of intercepting mbox.
					auto mbox_factory =
						[target_mbox = actual_consumer->so_direct_mbox()]
						( so_5::partially_constructed_agent_ptr_t /*agent_ptr*/,
							so_5::mbox_t source_mbox ) -> so_5::mbox_t
						{
							return { std::make_unique< intercepting_mbox_t >( source_mbox, target_mbox ) };
						};

					// Original consumer has to be created manually because we
					// will prepare a context for it by outselves.
					auto * original_consumer = coop.add_agent(
							std::make_unique< a_consumer_t >(
									so_5::agent_context_t( coop.environment() )
										+ so_5::agent_t::custom_direct_mbox_factory( mbox_factory ),
									"original" ) );

					// Now we can create producer and pass a reference to the
					// original consumer to it.
					coop.make_agent< a_producer_t >( *original_consumer );
				} );
		} );
}

int main()
{
	try
	{
		// First run: normal scenario with producer and consumer only.
		run_normal_scenario();

		// Second run: interception of msg_second.
		run_intercepting_scenario();
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

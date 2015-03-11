/*
 * An example of using message limits with redirection and
 * transformation for processing of message peaks.
 */

#include <iostream>
#include <random>
#include <deque>
#include <chrono>
#include <string>
#include <sstream>

#include <so_5/all.hpp>

// Helper class with fatilities to random numbers generation.
class random_generator_mixin_t
{
public :
	random_generator_mixin_t()
	{
		m_random_engine.seed( std::hash<decltype(this)>()(this) );
	}

	int
	random( int l, int h )
	{
		return std::uniform_int_distribution<>( l, h )( m_random_engine );
	}

private :
	// Engine for random values generation.
	std::default_random_engine m_random_engine;
};

// A request to be processed.
struct request : public so_5::rt::message_t
{
	// Return address.
	so_5::rt::mbox_t m_reply_to;
	// Request ID.
	int m_id;
	// Some payload.
	int m_payload;

	request(
		so_5::rt::mbox_t reply_to,
		int id,
		int payload )
		:	m_reply_to( std::move( reply_to ) )
		,	m_id( id )
		,	m_payload( payload )
	{}
};

// Typedef for smart intrusive pointer to request object.
typedef so_5::intrusive_ptr_t< request > request_smart_ptr_t;

// A reply to processed request.
struct reply : public so_5::rt::message_t
{
	// Request ID.
	int m_id;
	// Was request processed successfully?
	bool m_processed;

	reply( int id, int processed )
		:	m_id( id )
		,	m_processed( processed )
	{}
};

// Message for logger.
struct log_message : public so_5::rt::message_t
{
	// Text to be logged.
	std::string m_what;

	log_message( std::string what )
		: m_what( std::move( what ) )
	{}
};

// Logger agent.
class a_logger_t : public so_5::rt::agent_t
{
public :
	a_logger_t( context_t ctx )
		:	so_5::rt::agent_t( ctx
				// Limit the count of messages.
				// Because we can't lost log messages the overlimit
				// must lead to application crash.
				+ limit_then_abort< log_message >( 100 ) )
		,	m_started_at( std::chrono::steady_clock::now() )
	{}

	virtual void
	so_define_agent() override
	{
		so_default_state().event(
			[this]( const log_message & evt ) {
				std::cout << "[+" << time_delta()
						<< "] -- " << evt.m_what << std::endl;
			} );
	}

private :
	const std::chrono::steady_clock::time_point m_started_at;

	std::string
	time_delta() const
	{
		auto now = std::chrono::steady_clock::now();

		std::ostringstream ss;
		ss << std::chrono::duration_cast< std::chrono::milliseconds >(
				now - m_started_at ).count() / 1000.0 << "ms";

		return ss.str();
	}
};

// Load generation agent.
class a_generator_t : public so_5::rt::agent_t,
	private random_generator_mixin_t
{
public :
	a_generator_t(
		// Environment to work in.
		context_t ctx,
		// Name of generator.
		std::string name,
		// Starting value for request ID generation.
		int id_starting_point,
		// Address of message processor.
		so_5::rt::mbox_t performer,
		// Address of logger.
		so_5::rt::mbox_t logger )
		:	so_5::rt::agent_t( ctx
				// Expect no more than just one next_turn signal.
				+ limit_then_drop< msg_next_turn >( 1 )
				// Limit the quantity of non-processed replies in the queue.
				+ limit_then_transform( 10,
					// All replies which do not fit to message queue
					// will be transformed to log_messages and sent
					// to logger.
					[this]( const reply & msg ) {
						return make_transformed< log_message >( m_logger,
								m_name + ": unable to process reply(" +
								std::to_string( msg.m_id ) + ")" );
					} ) )
		,	m_name( std::move( name ) )
		,	m_performer( std::move( performer ) )
		,	m_logger( std::move( logger ) )
		,	m_turn_pause( 250 )
		,	m_last_id( id_starting_point )
	{}

	virtual void
	so_define_agent() override
	{
		so_default_state()
			.event< msg_next_turn >( &a_generator_t::evt_next_turn )
			.event( &a_generator_t::evt_reply );
	}

	virtual void
	so_evt_start() override
	{
		// Start work cycle.
		so_5::send_to_agent< msg_next_turn >( *this );
	}

private :
	// Signal about start of the next turn.
	struct msg_next_turn : public so_5::rt::signal_t {};

	// Generator name.
	const std::string m_name;
	// Performer for the requests processing.
	const so_5::rt::mbox_t m_performer;
	// Logger.
	const so_5::rt::mbox_t m_logger;

	// Pause between working turns.
	const std::chrono::milliseconds m_turn_pause;

	// Last generated ID for request.
	int m_last_id;

	// Type of map from request ID to the request.
	typedef std::map< int, request_smart_ptr_t > request_map_t;

	// Request which were send but the replies form them are not received yet.
	request_map_t m_active_requests;

	void
	evt_next_turn()
	{
		// Create new requests if there is a room in active_requests.
		try_create_new_requests( 7u );

		// Active requests must be sent (for the first time or repeated).
		send_active_requests();

		// Wait for next turn and process replies.
		so_5::send_delayed_to_agent< msg_next_turn >( *this, m_turn_pause );
	}

	void
	evt_reply( const reply & evt )
	{
		so_5::send< log_message >( m_logger,
				m_name + ": reply received(" + std::to_string( evt.m_id ) +
				"), processed:" + std::to_string( evt.m_processed ) );

		// Remove request only if it was successfully processed.
		// If not it will be resent on the next turn.
		if( evt.m_processed )
			m_active_requests.erase( evt.m_id );
	}

	void
	try_create_new_requests( unsigned int requests )
	{
		while( m_active_requests.size() < requests )
		{
			auto id = ++m_last_id;
			m_active_requests[ id ] = request_smart_ptr_t(
					new request( so_direct_mbox(), id, random( 30, 100 ) ) );
		}
	}

	void
	send_active_requests()
	{
		for( auto & r : m_active_requests )
		{
			so_5::send< log_message >( m_logger,
					m_name + ": sending request(" + std::to_string( r.first ) + ")" );
			m_performer->deliver_message( r.second );
		}
	}
};

// Performer agent.
class a_performer_t : public so_5::rt::agent_t
{
public :
	// A special indicator that agent must work with anotner performer.
	struct next_performer { so_5::rt::mbox_t m_target; };

	// A special indicator that agent is last in the chain.
	struct last_performer {};

	// Constructor for the case when worker is last in the chain.
	a_performer_t(
		context_t ctx,
		std::string name,
		last_performer,
		so_5::rt::mbox_t logger )
		:	so_5::rt::agent_t( ctx
				// Limit count of requests in the queue.
				// If queue is full then request must be transformed
				// to negative reply.
				+ limit_then_transform( 3,
					[]( const request & evt ) {
						return make_transformed< reply >( evt.m_reply_to,
								evt.m_id, false );
					} ) )
		,	m_name( std::move( name ) )
		,	m_logger( std::move( logger ) )
	{}

	// Constructor for the case when worker has the next performer in chain.
	a_performer_t(
		context_t ctx,
		std::string name,
		next_performer next,
		so_5::rt::mbox_t logger )
		:	so_5::rt::agent_t( ctx
				// Limit count of requests in the queue.
				// If queue is full then request must be redirected to the
				// next performer in the chain.
				+ limit_then_redirect< request >( 3,
					[next] { return next.m_target; } ) )
		,	m_name( std::move( name ) )
		,	m_logger( std::move( logger ) )
	{}

	virtual void
	so_define_agent() override
	{
		so_default_state().event( &a_performer_t::evt_request );
	}

private :
	const std::string m_name;
	const so_5::rt::mbox_t m_logger;

	void
	evt_request( const request & evt )
	{
		so_5::send< log_message >( m_logger,
				m_name + ": processing request(" +
				std::to_string( evt.m_id ) + ") for " +
				std::to_string( evt.m_payload ) + "ms" );

		// Imitation of some intensive processing.
		std::this_thread::sleep_for(
				std::chrono::milliseconds( evt.m_payload ) );

		// Generator must receive a reply for the request.
		so_5::send< reply >( evt.m_reply_to, evt.m_id, true );
	}
};

void
init( so_5::rt::environment_t & env )
{
	auto coop = env.create_coop( so_5::autoname );

	// Logger will work on the default dispatcher.
	auto logger = coop->make_agent< a_logger_t >();

	// Chain of performers.
	// Must work on dedicated thread_pool dispatcher.
	auto performer_disp = so_5::disp::thread_pool::create_private_disp( 3 );
	auto performer_binding_params = so_5::disp::thread_pool::params_t{}
			.fifo( so_5::disp::thread_pool::fifo_t::individual );

	// Start chain from the last agent.
	auto p3 = coop->make_agent_with_binder< a_performer_t >(
			performer_disp->binder( performer_binding_params ),
			"p3",
			a_performer_t::last_performer{},
			logger->so_direct_mbox() );
	auto p2 = coop->make_agent_with_binder< a_performer_t >(
			performer_disp->binder( performer_binding_params ),
			"p2",
			a_performer_t::next_performer{ p3->so_direct_mbox() },
			logger->so_direct_mbox() );
	auto p1 = coop->make_agent_with_binder< a_performer_t >(
			performer_disp->binder( performer_binding_params ),
			"p1",
			a_performer_t::next_performer{ p2->so_direct_mbox() },
			logger->so_direct_mbox() );

	// Generators will work on dedicated thread_pool dispatcher.
	auto generator_disp = so_5::disp::thread_pool::create_private_disp( 2 );
	auto generator_binding_params = so_5::disp::thread_pool::params_t{}
			.fifo( so_5::disp::thread_pool::fifo_t::individual );

	coop->make_agent_with_binder< a_generator_t >(
			generator_disp->binder( generator_binding_params ),
			"g1",
			0,
			p1->so_direct_mbox(),
			logger->so_direct_mbox() );
	coop->make_agent_with_binder< a_generator_t >(
			generator_disp->binder( generator_binding_params ),
			"g2",
			1000000,
			p1->so_direct_mbox(),
			logger->so_direct_mbox() );

	env.register_coop( std::move( coop ) );

	// Take some time to work.
	std::this_thread::sleep_for( std::chrono::seconds(5) );

	env.stop();
}

int main()
{
	try
	{
		so_5::launch( &init );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


/*
 * An example of using message limits with redirection and
 * transformation for processing of message peaks.
 */

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <deque>
#include <iostream>
#include <sstream>
#include <string>
#include <random>

#include <so_5/all.hpp>

// A request to be processed.
struct request
{
	// Return address.
	so_5::mbox_t m_reply_to;
	// Request ID.
	int m_id;
	// Some payload.
	int m_payload;
};

// A reply to processed request.
struct reply
{
	// Request ID.
	int m_id;
	// Was request processed successfully?
	bool m_processed;
};

// Message for logger.
struct log_message
{
	// Text to be logged.
	std::string m_what;
};

// Logger agent.
class a_logger_t : public so_5::agent_t
{
public :
	a_logger_t( context_t ctx )
		:	so_5::agent_t( ctx
				// Limit the count of messages.
				// Because we can't lost log messages the overlimit
				// must lead to application crash.
				+ limit_then_abort< log_message >( 100 ) )
		,	m_started_at( std::chrono::steady_clock::now() )
	{}

	virtual void so_define_agent() override
	{
		so_default_state().event(
			[this]( const log_message & evt ) {
				std::cout << "[+" << time_delta()
						<< "] -- " << evt.m_what << std::endl;
			} );
	}

private :
	const std::chrono::steady_clock::time_point m_started_at;

	std::string time_delta() const
	{
		auto now = std::chrono::steady_clock::now();

		std::ostringstream ss;
		ss << std::chrono::duration_cast< std::chrono::milliseconds >(
				now - m_started_at ).count() / 1000.0 << "ms";

		return ss.str();
	}
};

// Load generation agent.
class a_generator_t : public so_5::agent_t
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
		so_5::mbox_t performer,
		// Address of logger.
		so_5::mbox_t logger )
		:	so_5::agent_t( ctx
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

	virtual void so_define_agent() override
	{
		so_default_state()
			.event< msg_next_turn >( &a_generator_t::evt_next_turn )
			.event( &a_generator_t::evt_reply );
	}

	virtual void so_evt_start() override
	{
		// Start work cycle.
		so_5::send< msg_next_turn >( *this );
	}

private :
	// Signal about start of the next turn.
	struct msg_next_turn : public so_5::signal_t {};

	// Generator name.
	const std::string m_name;
	// Performer for the requests processing.
	const so_5::mbox_t m_performer;
	// Logger.
	const so_5::mbox_t m_logger;

	// Pause between working turns.
	const std::chrono::milliseconds m_turn_pause;

	// Last generated ID for request.
	int m_last_id;

	void evt_next_turn()
	{
		// Create and send new requests.
		generate_new_requests( random( 5, 8 ) );

		// Wait for next turn and process replies.
		so_5::send_delayed< msg_next_turn >( *this, m_turn_pause );
	}

	void evt_reply( const reply & evt )
	{
		so_5::send< log_message >( m_logger,
				m_name + ": reply received(" + std::to_string( evt.m_id ) +
				"), processed:" + std::to_string( evt.m_processed ) );
	}

	void generate_new_requests( int requests )
	{
		for(; requests > 0; --requests )
		{
			auto id = ++m_last_id;

			so_5::send< log_message >( m_logger,
					m_name + ": sending request(" + std::to_string( id ) + ")" );

			so_5::send< request >( m_performer,
					so_direct_mbox(), id, random( 30, 100 ) );
		}
	}

	static int random( int l, int h )
	{
		std::random_device rd;
		std::mt19937 gen{ rd() };
		return std::uniform_int_distribution< int >{l, h}(gen);
	}
};

// Performer agent.
class a_performer_t : public so_5::agent_t
{
public :
	// A special indicator that agent must work with anotner performer.
	struct next_performer { so_5::mbox_t m_target; };

	// A special indicator that agent is last in the chain.
	struct last_performer {};

	// Constructor for the case when worker is last in the chain.
	a_performer_t(
		context_t ctx,
		std::string name,
		float slowdown,
		last_performer,
		so_5::mbox_t logger )
		:	so_5::agent_t( ctx
				// Limit count of requests in the queue.
				// If queue is full then request must be transformed
				// to negative reply.
				+ limit_then_transform( 3,
					[]( const request & evt ) {
						return make_transformed< reply >( evt.m_reply_to,
								evt.m_id, false );
					} ) )
		,	m_name( std::move( name ) )
		,	m_slowdown( slowdown )
		,	m_logger( std::move( logger ) )
	{}

	// Constructor for the case when worker has the next performer in chain.
	a_performer_t(
		context_t ctx,
		std::string name,
		float slowdown,
		next_performer next,
		so_5::mbox_t logger )
		:	so_5::agent_t( ctx
				// Limit count of requests in the queue.
				// If queue is full then request must be redirected to the
				// next performer in the chain.
				+ limit_then_redirect< request >( 3,
					[next] { return next.m_target; } ) )
		,	m_name( std::move( name ) )
		,	m_slowdown( slowdown )
		,	m_logger( std::move( logger ) )
	{}

	virtual void so_define_agent() override
	{
		so_default_state().event( &a_performer_t::evt_request );
	}

private :
	const std::string m_name;
	const float m_slowdown;
	const so_5::mbox_t m_logger;

	void evt_request( const request & evt )
	{
		// Processing time is depend on speed of the performer.
		auto processing_time = static_cast< int >(
				m_slowdown * evt.m_payload );

		so_5::send< log_message >( m_logger,
				m_name + ": processing request(" +
				std::to_string( evt.m_id ) + ") for " +
				std::to_string( processing_time ) + "ms" );

		// Imitation of some intensive processing.
		std::this_thread::sleep_for(
				std::chrono::milliseconds( processing_time ) );

		// Generator must receive a reply for the request.
		so_5::send< reply >( evt.m_reply_to, evt.m_id, true );
	}
};

void init( so_5::environment_t & env )
{
	env.introduce_coop( [&env]( so_5::coop_t & coop ) {
		// Logger will work on the default dispatcher.
		auto logger = coop.make_agent< a_logger_t >();

		// Chain of performers.
		// Must work on dedicated thread_pool dispatcher.
		auto performer_disp = so_5::disp::thread_pool::create_private_disp( env, 3 );
		auto performer_binding_params = so_5::disp::thread_pool::bind_params_t{}
				.fifo( so_5::disp::thread_pool::fifo_t::individual );

		// Start chain from the last agent.
		auto p3 = coop.make_agent_with_binder< a_performer_t >(
				performer_disp->binder( performer_binding_params ),
				"p3",
				1.4f, // Each performer in chain is slower then previous.
				a_performer_t::last_performer{},
				logger->so_direct_mbox() );
		auto p2 = coop.make_agent_with_binder< a_performer_t >(
				performer_disp->binder( performer_binding_params ),
				"p2",
				1.2f, // Each performer in chain is slower then previous.
				a_performer_t::next_performer{ p3->so_direct_mbox() },
				logger->so_direct_mbox() );
		auto p1 = coop.make_agent_with_binder< a_performer_t >(
				performer_disp->binder( performer_binding_params ),
				"p1",
				1.0f, // The first performer is the fastest one.
				a_performer_t::next_performer{ p2->so_direct_mbox() },
				logger->so_direct_mbox() );

		// Generators will work on dedicated thread_pool dispatcher.
		auto generator_disp = so_5::disp::thread_pool::create_private_disp( env, 2 );
		auto generator_binding_params = so_5::disp::thread_pool::bind_params_t{}
				.fifo( so_5::disp::thread_pool::fifo_t::individual );

		coop.make_agent_with_binder< a_generator_t >(
				generator_disp->binder( generator_binding_params ),
				"g1",
				0,
				p1->so_direct_mbox(),
				logger->so_direct_mbox() );
		coop.make_agent_with_binder< a_generator_t >(
				generator_disp->binder( generator_binding_params ),
				"g2",
				1000000,
				p1->so_direct_mbox(),
				logger->so_direct_mbox() );
	});

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


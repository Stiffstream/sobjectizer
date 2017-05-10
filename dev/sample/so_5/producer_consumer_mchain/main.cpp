/*
 * An example of using mchain for solving producer/consumer problem.
 *
 * Several producers will send requests to the single consumer. All requests
 * will be sent to size-limited mchain with a timeout on overflow.
 * This timeout will slowdown producers.
 *
 * The consumer will periodically process requests from mchain and sends
 * replies back. A very simple not-empty notificator will be used for
 * mchain to inform the consumer about presense of new requests.
 */

#include <iostream>
#include <chrono>
#include <random>

#include <so_5/all.hpp>

using steady_clock = std::chrono::steady_clock;

//
// Stuff for logging.
//
using log_msg = std::string;

so_5::mbox_t make_logger( so_5::coop_t & coop )
{
	// Logger will work on its own working thread.
	auto logger = coop.define_agent(
			so_5::disp::one_thread::create_private_disp(
					coop.environment() )->binder() );

	logger.event( logger, []( const log_msg & msg ) {
			std::cout << msg << std::endl;
		} );

	return logger.direct_mbox();
}

template< typename A >
inline void operator<<=( const so_5::mbox_t & to, A && a )
{
	so_5::send< log_msg >( to, std::forward< A >(a) );
}

struct msg_maker
{
	std::ostringstream m_os;

	template< typename A >
	msg_maker & operator<<( A && a ) 
	{
		m_os << std::forward< A >(a);
		return *this;
	}
};

inline void operator<<=( const so_5::mbox_t & to, msg_maker & maker )
{
	to <<= maker.m_os.str();
}

//
// Implementation of example shutdowner.
//
/*
 * Shutdowner will shutdown the SObjectizer Environment when all
 * producers send finish signals.
 */
class shutdowner final : public so_5::agent_t
{
	struct another_producer_finished : public so_5::signal_t {};

public :
	shutdowner( context_t ctx, unsigned int producers_count )
		:	so_5::agent_t{ ctx }
		,	m_producers_left{ producers_count }
	{
		so_subscribe( so_environment().create_mbox( "shutdowner" ) )
			.event< another_producer_finished >( [this] {
					--m_producers_left;
					if( !m_producers_left )
						so_environment().stop();
				} );
	}

	static void producer_finished( const so_5::agent_t & p )
	{
		so_5::send< another_producer_finished >(
				p.so_environment().create_mbox( "shutdowner" ) );
	}

private :
	unsigned int m_producers_left;
};

//
// Implementation of producers.
//

// A request to be sent for processing.
struct request
{
	so_5::mbox_t m_who;
	std::string m_payload;
};

// A repsonse from consumer.
struct reply
{
	std::string m_payload;
};

/*
 * Producer agent will send N requests and then initiate finish signal.
 */
class producer final : public so_5::agent_t
{
	// This signal allows to send next request for consumer.
	struct send_next : public so_5::signal_t {};

public :
	producer( context_t ctx,
		std::string name, 
		so_5::mbox_t logger_mbox,
		so_5::mbox_t consumer_mbox,
		unsigned int requests )
		:	so_5::agent_t{ ctx }
		,	m_name( std::move(name) )
		,	m_logger_mbox{ std::move(logger_mbox) }
		,	m_consumer_mbox{ std::move(consumer_mbox) }
		,	m_requests_left{ requests }
	{
		so_subscribe_self()
			.event( &producer::evt_reply )
			.event< send_next >( &producer::evt_send_next );
	}

	virtual void so_evt_start() override
	{
		// Initiate request sending loop.
		so_5::send< send_next >( *this );
	}

private :
	const std::string m_name;
	const so_5::mbox_t m_logger_mbox;
	const so_5::mbox_t m_consumer_mbox;

	unsigned int m_requests_left;

	// An event for next attempt to send another requests.
	void evt_send_next()
	{
		if( m_requests_left )
		{
			// Send can wait on full mchain. Mark the start time to
			// calculate send call duration later.
			const auto started_at = steady_clock::now();
			try
			{
				// Send another request.
				// Note: this call can wait on full mchain.
				so_5::send< request >( m_consumer_mbox,
						so_direct_mbox(),
						m_name + "_request_" + std::to_string( m_requests_left ) );

				// How much time the send take?
				const auto ms = std::chrono::duration_cast<
						std::chrono::milliseconds >( steady_clock::now()
								- started_at ).count();

				m_logger_mbox <<= msg_maker() << m_name << ": request sent in "
						<< ms << "ms";
			}
			catch( const so_5::exception_t & ex )
			{
				// Log the reason of send request failure.
				m_logger_mbox <<= msg_maker() << m_name << ": request NOT SENT, "
						<< ex.what();

				// Initiate next send attempt.
				so_5::send< send_next >( *this );
			}
		}
		else
			// No more requests to send. Shutdowner must known about it.
			shutdowner::producer_finished( *this );
	}

	void evt_reply( const reply & msg )
	{
		m_logger_mbox <<= msg_maker() << m_name << ": reply received, "
				<< msg.m_payload;

		--m_requests_left;

		so_5::send< send_next >( *this );
	}
};

//
// Implementation of consumer.
//
class consumer final : public so_5::agent_t
{
	// This signal will be sent by not_empty_notificator when
	// the first message is stored to the empty mchain.
	struct chain_has_requests : public so_5::signal_t {};

public :
	consumer( context_t ctx, so_5::mbox_t logger_mbox )
		:	so_5::agent_t{ ctx + limit_then_drop< chain_has_requests >(1) }
		,	m_logger_mbox{ std::move(logger_mbox) }
	{
		// Appropriate mchain must be created.
		m_chain = so_environment().create_mchain(
			so_5::make_limited_with_waiting_mchain_params(
				// No more than 10 requests in the chain.
				10,
				// Preallocated storage for the chain.
				so_5::mchain_props::memory_usage_t::preallocated,
				// Throw an exception on overload.
				so_5::mchain_props::overflow_reaction_t::throw_exception,
				// Wait no more than 0.5s on overflow.
				std::chrono::milliseconds(150) )
			// A custom notificator must be used for chain.
			.not_empty_notificator( [this] {
					so_5::send< chain_has_requests >( *this );
				} ) );

		so_subscribe_self().event< chain_has_requests >(
				&consumer::process_requests );
	}

	// A mchain will look like an ordinary mbox from outside of consumer.
	so_5::mbox_t consumer_mbox() const
	{
		return m_chain->as_mbox();
	}

private :
	const so_5::mbox_t m_logger_mbox;
	so_5::mchain_t m_chain;

	void process_requests()
	{
		auto r = receive(
				// Handle no more than 5 requests at once.
				// No wait if queue is empty.
				from( m_chain ).handle_n( 5 ).no_wait_on_empty(),
				[]( const request & req ) {
					// Imitation of some hardwork before sending a reply back.
					std::this_thread::sleep_for( random_pause() );
					so_5::send< reply >( req.m_who, req.m_payload + "#handled" );
				} );
		m_logger_mbox <<= msg_maker()
				<< "=== " << r.handled() << " request(s) handled";

		if( !m_chain->empty() )
			// Not all messages from chain have been processed.
			// Initiate new processing by sending the signal to itself.
			so_5::send< chain_has_requests >( *this );
	}

	static std::chrono::milliseconds
	random_pause()
	{
		std::random_device rd;
		std::mt19937 gen{ rd() };
		return std::chrono::milliseconds(
				std::uniform_int_distribution< unsigned int >{2u, 25u}(gen) );
	}
};

void fill_demo_coop( so_5::coop_t & coop )
{
	const unsigned int producers = 40;

	// Shutdowner will work on the default dispatcher.
	coop.make_agent_with_binder< shutdowner >(
			so_5::make_default_disp_binder( coop.environment() ),
			producers );

	// Logger will work on its own context.
	const auto logger_mbox = make_logger( coop );

	// Consumer agent must be created first.
	// It will work on its own working thread.
	auto consumer_mbox = coop.make_agent_with_binder< consumer >(
			so_5::disp::one_thread::create_private_disp(
					coop.environment() )->binder(),
			logger_mbox )->consumer_mbox();

	// All producers will work on thread pool dispatcher.
	namespace tp_disp = so_5::disp::thread_pool;
	auto disp = tp_disp::create_private_disp( coop.environment() );
	// All agents on this dispatcher will have independent event queues.
	const auto bind_params = tp_disp::bind_params_t{}
			.fifo( tp_disp::fifo_t::individual )
			.max_demands_at_once( 1 );

	// All producers can be created now.
	for( unsigned int i = 0; i != producers; ++i )
		coop.make_agent_with_binder< producer >(
				disp->binder( bind_params ),
				"producer-" + std::to_string( i + 1 ),
				logger_mbox,
				consumer_mbox,
				10u );
}

int main()
{
	try
	{
		so_5::launch( []( so_5::environment_t & env ) {
			env.introduce_coop( fill_demo_coop );
		} );

		return 0;
	}
	catch( const std::exception & x )
	{
		std::cerr << "Exception: " << x.what() << std::endl;
	}

	return 2;
}


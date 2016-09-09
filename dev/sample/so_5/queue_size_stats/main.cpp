/*
 * An example of using SObjectizer-5.5.4 features for receiving
 * run-time monitoring information.
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

// A signal to worker agent to do something.
struct msg_start_thinking : public so_5::signal_t {};

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

// Agent for receiving run-time monitoring information.
class a_stats_listener_t : public so_5::agent_t
{
public :
	a_stats_listener_t(
		// Environment to work in.
		context_t ctx,
		// Address of logger.
		so_5::mbox_t logger )
		:	so_5::agent_t( ctx )
		,	m_logger( std::move( logger ) )
	{}

	virtual void so_define_agent() override
	{
		using namespace so_5::stats;

		auto & controller = so_environment().stats_controller();

		// Set up a filter for messages with run-time monitoring information.
		so_set_delivery_filter(
			// Message box to which delivery filter must be set.
			controller.mbox(),
			// Delivery predicate.
			[]( const messages::quantity< std::size_t > & msg ) {
				// Process only messages related to dispatcher's queue sizes.
				return suffixes::work_thread_queue_size() == msg.m_suffix;
			} );

		// We must receive messages from run-time monitor.
		so_default_state()
			.event(
				// This is mbox to that run-time statistic will be sent.
				controller.mbox(),
				&a_stats_listener_t::evt_quantity )
			.event( controller.mbox(),
				[this]( const messages::distribution_started & ) {
					so_5::send< log_message >( m_logger, "--- DISTRIBUTION STARTED ---" );
				} )
			.event( controller.mbox(),
				[this]( const messages::distribution_finished & ) {
					so_5::send< log_message >( m_logger, "--- DISTRIBUTION FINISHED ---" );
				} );
	}

	virtual void so_evt_start() override
	{
		// Change the speed of run-time monitor updates.
		so_environment().stats_controller().set_distribution_period(
				std::chrono::milliseconds( 330 ) );
		// Turn the run-timer monitoring on.
		so_environment().stats_controller().turn_on();
	}

private :
	const so_5::mbox_t m_logger;

	void evt_quantity(
		const so_5::stats::messages::quantity< std::size_t > & evt )
	{
		std::ostringstream ss;

		ss << "stats: '" << evt.m_prefix << evt.m_suffix << "': " << evt.m_value;

		so_5::send< log_message >( m_logger, ss.str() );
	}
};

// Load generation agent.
class a_generator_t : public so_5::agent_t
{
public :
	a_generator_t(
		// Environment to work in.
		context_t ctx,
		// Address of logger.
		so_5::mbox_t logger,
		// Addresses of worker agents.
		std::vector< so_5::mbox_t > workers )
		:	so_5::agent_t( ctx )
		,	m_logger( std::move( logger ) )
		,	m_workers( std::move( workers ) )
		,	m_turn_pause( 600 )
	{}

	virtual void so_define_agent() override
	{
		so_default_state()
			.event< msg_next_turn >( &a_generator_t::evt_next_turn );
	}

	virtual void so_evt_start() override
	{
		// Start work cycle.
		so_5::send< msg_next_turn >( *this );
	}

private :
	// Signal about start of the next turn.
	struct msg_next_turn : public so_5::signal_t {};

	// Logger.
	const so_5::mbox_t m_logger;

	// Workers.
	const std::vector< so_5::mbox_t > m_workers;

	// Pause between working turns.
	const std::chrono::milliseconds m_turn_pause;

	void evt_next_turn()
	{
		// Create and send new requests.
		generate_new_requests( random( 100, 200 ) );

		// Wait for next turn and process replies.
		so_5::send_delayed< msg_next_turn >( *this, m_turn_pause );
	}

	void generate_new_requests( unsigned int requests )
	{
		const auto size = m_workers.size();

		for( unsigned int i = 0; i != requests; ++i )
			so_5::send< msg_start_thinking >( m_workers[ i % size ] );

		so_5::send< log_message >( m_logger,
				std::to_string( requests ) + " requests are sent" );
	}

	static unsigned int
	random( unsigned int left, unsigned int right )
	{
		std::random_device rd;
		std::mt19937 gen{ rd() };
		return std::uniform_int_distribution< unsigned int >{left, right}(gen);
	}
};

// Worker agent.
class a_worker_t : public so_5::agent_t
{
public :
	a_worker_t( context_t ctx )
		:	so_5::agent_t( ctx
				// Limit the maximum count of messages.
				+ limit_then_drop< msg_start_thinking >( 50 ) )
	{}

	virtual void so_define_agent() override
	{
		so_default_state().event< msg_start_thinking >( [] {
				std::this_thread::sleep_for(
					std::chrono::milliseconds( 10 ) );
			} );
	}
};

void init( so_5::environment_t & env )
{
	env.introduce_coop( [&env]( so_5::coop_t & coop ) {
		// Logger will work on the default dispatcher.
		auto logger = coop.make_agent< a_logger_t >();

		// Run-time stats listener will work on a dedicated
		// one-thread dispatcher.
		coop.make_agent_with_binder< a_stats_listener_t >(
				so_5::disp::one_thread::create_private_disp(
						env, "stats_listener" )->binder(),
				logger->so_direct_mbox() );

		// Bunch of workers.
		// Must work on dedicated thread_pool dispatcher.
		auto worker_disp = so_5::disp::thread_pool::create_private_disp(
				env,
				3, // Count of working threads.
				"workers" ); // Name of dispatcher (for convience of monitoring).
		const auto worker_binding_params = so_5::disp::thread_pool::bind_params_t{}
				.fifo( so_5::disp::thread_pool::fifo_t::individual );

		std::vector< so_5::mbox_t > workers;
		for( int i = 0; i != 5; ++i )
		{
			auto w = coop.make_agent_with_binder< a_worker_t >(
					worker_disp->binder( worker_binding_params ) );
			workers.push_back( w->so_direct_mbox() );
		}

		// Generators will work on dedicated active_obj dispatcher.
		auto generator_disp = so_5::disp::active_obj::create_private_disp(
				env, "generator" );

		coop.make_agent_with_binder< a_generator_t >(
				generator_disp->binder(),
				logger->so_direct_mbox(),
				std::move( workers ) );
	});

	// Take some time to work.
	std::this_thread::sleep_for( std::chrono::seconds(50) );

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


/*
 * A simple example of work load generation and simple form
 * of overload control based on SObjectizer features.
 */

#include <iostream>
#include <random>

#include <so_5/all.hpp>

// A helpers for trace messages.
std::mutex g_trace_mutex;
class locker_t
{
public :
	locker_t( std::mutex & m ) : m_lock( m ) {}
	locker_t( locker_t && o ) : m_lock( std::move( o.m_lock ) ) {}
	operator bool() const { return true; }
private :
	std::unique_lock< std::mutex > m_lock;
};

#define TRACE() \
if( auto l = locker_t{ g_trace_mutex } ) std::cout  

// Helper class with fatilities to random numbers generation.
class random_generator_mixin_t
{
public :
	random_generator_mixin_t()
	{
		m_random_engine.seed();
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

// Message to be processed by worker agent.
struct application_request : public so_5::message_t
{
	std::string m_to;
	std::string m_from;
	std::string m_payload;
	std::string m_attributes;
	std::string m_generator;

	application_request(
		std::string to,
		std::string from,
		std::string payload,
		std::string attributes,
		std::string generator )
		:	m_to( std::move( to ) )
		,	m_from( std::move( from ) )
		,	m_payload( std::move( payload ) )
		,	m_attributes( std::move( attributes ) )
		,	m_generator( std::move( generator ) )
	{}
};

// Load generation agent.
class a_generator_t : public so_5::agent_t,
	private random_generator_mixin_t
{
public :
	a_generator_t(
		// Environment to work in.
		context_t ctx,
		// Name of generator.
		std::string name,
		// Workers.
		const std::vector< so_5::mbox_t > & workers_mboxes )
		:	so_5::agent_t( ctx )
		,	m_name( std::move( name ) )
		,	m_workers_mboxes( workers_mboxes )
	{}

	virtual void so_define_agent() override
	{
		// Just one handler in one state.
		so_default_state().event< msg_next_turn >(
				&a_generator_t::evt_next_turn );
	}

	virtual void so_evt_start() override
	{
		// Start work cycle.
		so_5::send< msg_next_turn >( *this );
	}

private :
	// Signal about next turn start.
	struct msg_next_turn : public so_5::signal_t {};

	// Generator name.
	const std::string m_name;
	// Workers.
	const std::vector< so_5::mbox_t > m_workers_mboxes;

	void evt_next_turn()
	{
		// How many requests will be sent on this turn.
		const int requests = random( 1, 100 );

		TRACE() << "GEN(" << m_name << ") turn started, requests="
				<< requests << std::endl;

		// We need copy of workers list to modify it if
		// some worker rejects our requests.
		std::vector< so_5::mbox_t > live_workers( m_workers_mboxes );
		int sent = 0;
		// If there is no active workers there is no need to continue.
		while( sent < requests && !live_workers.empty() )
		{
			if( generate_next_request( live_workers ) )
				++sent;
		}

		// How much to sleep until next turn.
		const auto next_turn_pause = std::chrono::milliseconds( random(0, 50) );

		TRACE() << "GEN(" << m_name << ") requests generated="
				<< sent << ", will sleep for "
				<< next_turn_pause.count() << "ms" << std::endl;

		so_5::send_delayed< msg_next_turn >( *this, next_turn_pause );
	}

	bool generate_next_request( std::vector< so_5::mbox_t > & workers )
	{
		auto it = workers.begin();
		if( workers.size() > 1 )
			// There are more then one live workers. Select one randomly.
			std::advance( it, random( 0, static_cast< int >(workers.size()) - 1 ) );

		// Prepare request.
		auto request = std::unique_ptr< application_request >(
				new application_request(
						"Mr.Alexander Graham Bell",
						"Mr.Thomas A. Watson",
						"Mr. Watson - Come here - I want to see you",
						"BestEffort,InMemory,NormalPriority",
						m_name ) );

		// Send it to worker.
		auto result = push_request_to_receiver( *it, std::move( request ) );
		if( !result )
			workers.erase( it );

		return result;
	}

	bool push_request_to_receiver(
		const so_5::mbox_t & to,
		std::unique_ptr< application_request > req )
	{
		// There is a plenty of room for any errors related to
		// synchronous invocation. Catch them all and treat them
		// as inabillity of worker to process request.
		try
		{
			return to->get_one< bool >()
					.wait_for( std::chrono::milliseconds( 10 ) )
					.sync_get( std::move( req ) );
		}
		catch( const std::exception & x )
		{
			TRACE()<< "GEN(" << m_name << ") failed to push request: "
					<< x.what() << std::endl;
		}

		return false;
	}
};

// Load receiver agent.
class a_receiver_t : public so_5::agent_t
{
public :
	// A signal to take the collected requests to processor.
	struct msg_take_requests : public so_5::signal_t {};

	a_receiver_t(
		// Environment to work in.
		context_t ctx,
		// Receiver's name.
		std::string name,
		// Max capacity of receiver
		std::size_t max_receiver_capacity )
		:	so_5::agent_t( ctx )
		,	m_name( std::move( name ) )
		,	max_capacity( max_receiver_capacity )
	{
		m_requests.reserve( max_capacity );
	}

	virtual void so_define_agent() override
	{
		this >>= st_not_full;

		// When in the normal state...
		st_not_full
			// Store new request in ordinary way...
			.event( &a_receiver_t::evt_store_request )
			// Return request array to processor.
			.event< msg_take_requests >( &a_receiver_t::evt_take_requests );

		// When overload...
		st_overload
			// Reject any new request...
			.event( &a_receiver_t::evt_reject_request )
			// But return request array to processer as usual.
			.event< msg_take_requests >( &a_receiver_t::evt_take_requests );
	}

private :
	const state_t st_not_full{ this };
	const state_t st_overload{ this };

	// Receiver's name.
	const std::string m_name;

	// Max count of items to store between processing turns.
	const std::size_t max_capacity;

	// Storage for requests between turns.
	std::vector< application_request > m_requests;

	bool evt_store_request( const application_request & what )
	{
		// Just store new request.
		m_requests.push_back( what );

		if( m_requests.size() < max_capacity )
			// Not overloaded, new requests could be accepted.
			return true;
		else
		{
			// Overloaded. New requests will be rejected.
			this >>= st_overload;
			return false;
		}
	}

	bool evt_reject_request( const application_request & what )
	{
		TRACE() << "REC(" << m_name << ") reject request from "
				<< what.m_generator << std::endl;
		return false;
	}

	std::vector< application_request >
	evt_take_requests()
	{
		// Value to return.
		std::vector< application_request > result;
		result.swap( m_requests );

		TRACE() << "REC(" << m_name << ") takes requests off, count: "
				<< result.size() << std::endl;

		// Preparation to the next turn.
		m_requests.reserve( max_capacity );
		this >>= st_not_full;

		// Return request array to producer.
		return result;
	}
};

// Load processor agent.
class a_processor_t : public so_5::agent_t,
	private random_generator_mixin_t
{
public :
	a_processor_t(
		// Environment to work in.
		context_t ctx,
		// Processor's name.
		std::string name,
		// Receiver mbox.
		const so_5::mbox_t & receiver )
		:	so_5::agent_t( ctx )
		,	m_name( std::move( name ) )
		,	m_receiver( receiver )
	{}

	virtual void so_define_agent() override
	{
		// Just one handler in the default state.
		so_default_state().event< msg_next_turn >(
				&a_processor_t::evt_next_turn );
	}

	virtual void so_evt_start() override
	{
		// Start working cycle.
		so_5::send< msg_next_turn >( *this );
	}

private :
	// Start of next processing turn signal.
	struct msg_next_turn : public so_5::signal_t {};

	// Processor name.
	const std::string m_name;

	// Receiver.
	const so_5::mbox_t m_receiver;

	void evt_next_turn()
	{
		// Take requests from receiver.
		auto requests = take_requests();

		if( requests.empty() )
		{
			// No requests. There is no sense to make next request
			// immediately. It is better to take some time to generators
			// and receiver.
			TRACE() << "PRO(" << m_name << ") no request received, sleeping"
					<< std::endl;
			so_5::send_delayed< msg_next_turn >(
					*this, std::chrono::milliseconds( 25 ) );
		}
		else
		{
			// There are some requests. They must be processed.
			process_requests( requests );
			// Start next turn immediately.
			so_5::send< msg_next_turn >( *this );
		}
	}

	std::vector< application_request >
	take_requests()
	{
		// There is a plenty of room for any errors related to
		// synchronous invocation. Catch them all and treat them
		// as inabillity of receiver to provide request array.
		try
		{
			return so_5::request_value<
						std::vector< application_request >,
						a_receiver_t::msg_take_requests >
					( m_receiver, std::chrono::milliseconds( 20 ) );
		}
		catch( const std::exception & x )
		{
			TRACE() << "PRO(" << m_name << ") failed to take requests: "
					<< x.what() << std::endl;
		}

		return std::vector< application_request >();
	}

	void process_requests( const std::vector< application_request > & requests )
	{
		TRACE() << "PRO(" << m_name << ") start processing, requests="
				<< requests.size() << std::endl;

		// Just imitation of requests processing.
		// Processing time is proportional to count of requests.
		const auto processing_time = std::chrono::microseconds(
				requests.size() * static_cast< unsigned int >(random( 150, 1500 )) );
		std::this_thread::sleep_for( processing_time );

		TRACE() << "PRO(" << m_name << ") processing took: "
				<< processing_time.count() / 1000.0 << "ms" << std::endl;
	}
};

std::vector< so_5::mbox_t >
create_processing_coops( so_5::environment_t & env )
{
	std::vector< so_5::mbox_t > result;

	std::size_t capacities[] = { 25, 35, 40, 15, 20 };

	// Private dispatcher for receivers.
	auto receiver_disp = so_5::disp::thread_pool::create_private_disp( env, 2 );
	// And private dispatcher for processors.
	auto processor_disp = so_5::disp::active_obj::create_private_disp( env );

	int i = 0;
	for( auto c : capacities )
	{
		env.introduce_coop( [&]( so_5::coop_t & coop ) {
			auto receiver = coop.make_agent_with_binder< a_receiver_t >(
					receiver_disp->binder( so_5::disp::thread_pool::bind_params_t{} ),
					"r" + std::to_string(i), c );

			const auto receiver_mbox = receiver->so_direct_mbox();
			result.push_back( receiver_mbox );

			coop.make_agent_with_binder< a_processor_t >(
					processor_disp->binder(),
					"p" + std::to_string(i), receiver_mbox );
		} );

		++i;
	}

	return result;
}

void init( so_5::environment_t & env )
{
	auto receivers = create_processing_coops( env );

	using namespace so_5::disp::thread_pool;

	// A private dispatcher for generators cooperation.
	auto generators_disp = create_private_disp( env, 3 );
	env.introduce_coop(
			generators_disp->binder( bind_params_t{}.fifo( fifo_t::individual ) ),
			[&receivers]( so_5::coop_t & coop ) {
				for( int i = 0; i != 3; ++i )
					coop.make_agent< a_generator_t >(
							"g" + std::to_string(i), receivers );
			} );

	// Taking some time for the agents.
	std::this_thread::sleep_for( std::chrono::seconds( 10 ) );
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


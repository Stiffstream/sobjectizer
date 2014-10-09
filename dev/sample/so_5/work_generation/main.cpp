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

// Message to be processed by worker agent.
struct application_request : public so_5::rt::message_t
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
class a_generator_t : public so_5::rt::agent_t
{
public :
	a_generator_t(
		// Environment to work in.
		so_5::rt::environment_t & env,
		// Name of generator.
		std::string name,
		// Workers.
		const std::vector< so_5::rt::mbox_ref_t > & workers_mboxes )
		:	so_5::rt::agent_t( env )
		,	m_name( std::move( name ) )
		,	m_workers_mboxes( workers_mboxes )
	{
		m_random_engine.seed( std::hash< std::string >()( m_name ) );
	}

	virtual void
	so_define_agent() override
	{
		so_default_state().handle< msg_next_turn >(
				&a_generator_t::evt_next_turn );
	}

	virtual void
	so_evt_start() override
	{
		so_5::send< msg_next_turn >( *this );
	}

private :
	// Signal about next turn start.
	struct msg_next_turn : public so_5::rt::signal_t {};

	// Generator name.
	const std::string m_name;
	// Workers.
	const std::vector< so_5::rt::mbox_ref_t > m_workers_mboxes;

	// Engine for random values generation.
	std::default_random_engine m_random_engine;

	// Working parameters.
	//
	// Max count of application_request per one turn.
	static const int max_application_requests_at_once = 100;
	// Max sleeping time for the next turn.
	// Milliseconds.
	static const int max_next_turn_sleeping_time = 50;

	void
	evt_next_turn()
	{
		const int requests = random( 1, max_application_requests_at_once );

		TRACE() << "GEN(" << m_name << ") turn started, requests="
				<< requests << std::endl;

		std::vector< so_5::rt::mbox_ref_t > live_workers( m_workers_mboxes );
		int sent = 0;
		while( sent < requests && !live_workers.empty() )
		{
			generate_next_request( live_workers );
			++sent;
		}

		const int next_turn_pause = random(
				0, max_next_turn_sleeping_time );

		TRACE() << "GEN(" << m_name << ") requests generated="
				<< sent << ", will sleep for "
				<< next_turn_pause << "ms" << std::endl;

		so_5::send_delayed< msg_next_turn >( *this, so_direct_mbox(),
				std::chrono::milliseconds( next_turn_pause ) );
	}

	void
	generate_next_request( std::vector< so_5::rt::mbox_ref_t > & workers )
	{
		auto it = workers.begin();
		if( workers.size() > 1 )
			std::advance( it, random( 0, workers.size() - 1 ) );

		so_5::send< application_request >( *it,
				"Mr.Alexander Graham Bell",
				"Mr.Thomas A. Watson",
				"Mr. Watson - Come here - I want to see you",
				"besteffort,inmemory,normalpriority",
				m_name );
	}

	int
	random( int low, int high )
	{
		return std::uniform_int_distribution<>( low, high )( m_random_engine );
	}
};

// Load receiver agent.
class a_load_receiver_t : public so_5::rt::agent_t
{
public :
	// A signal to take the collected requests to processor.
	struct msg_take_requests : public so_5::rt::signal_t {};

	a_load_receiver_t(
		// Environment to work in.
		so_5::rt::environment_t & env,
		// Receiver's name.
		std::string name,
		// Max capacity of receiver
		std::size_t max_receiver_capacity )
		:	so_5::rt::agent_t( env )
		,	m_name( name )
		,	max_capacity( max_receiver_capacity )
	{
		m_requests.reserve( max_capacity );
	}

	virtual void
	so_define_agent() override
	{
		this >>= st_not_full;

		st_not_full.handle( &a_load_receiver_t::evt_store_request )
			.handle< msg_take_requests >( &a_load_receiver_t::evt_take_requests );

		st_overload.handle( &a_load_receiver_t::evt_reject_request )
			.handle< msg_take_requests >( &a_load_receiver_t::evt_take_requests );
	}

private :
	const so_5::rt::state_t st_not_full = so_make_state();
	const so_5::rt::state_t st_overload = so_make_state();

	// Receiver's name.
	const std::string m_name;

	// Max count of items to store between processing turns.
	const std::size_t max_capacity;

	// Storage for requests between turns.
	std::vector< application_request > m_requests;

	bool
	evt_store_request( const application_request & what )
	{
		m_requests.push_back( what );

		if( m_requests.size() < max_capacity )
			return true;
		else
		{
			this >>= st_overload;
			return false;
		}
	}

	bool
	evt_reject_request( const application_request & what )
	{
		TRACE() << "REC(" << m_name << ") reject request from "
				<< what.m_generator << std::endl;
		return false;
	}

	std::vector< application_request >
	evt_take_requests()
	{
		std::vector< application_request > result;
		result.swap( m_requests );

		TRACE() << "REC(" << m_name << ") takes requests off, count: "
				<< result.size() << std::endl;

		// Preparation to the next turn.
		m_requests.reserve( max_capacity );
		this >>= st_not_full;

		return result;
	}
};

// Load processor agent.
class a_processor_t : public so_5::rt::agent_t
{
public :
	a_processor_t(
		// Environment to work in.
		so_5::rt::environment_t & env,
		// Processor's name.
		std::string name,
		// Receiver mbox.
		const so_5::rt::mbox_ref_t & receiver )
		:	so_5::rt::agent_t( env )
		,	m_name( std::move( name ) )
		,	m_receiver( receiver )
	{
		m_random_engine.seed( std::hash< std::string >()( m_name ) );
	}

	virtual void
	so_define_agent() override
	{
		so_default_state().handle< msg_next_turn >(
				&a_processor_t::evt_next_turn );
	}

	virtual void
	so_evt_start() override
	{
		evt_next_turn();
	}

private :
	// Start of next processing turn signal.
	struct msg_next_turn : public so_5::rt::signal_t {};

	// Processor name.
	const std::string m_name;

	// Receiver.
	const so_5::rt::mbox_ref_t m_receiver;

	// Engine for random values generation.
	std::default_random_engine m_random_engine;

	const std::chrono::milliseconds next_turn_sleep_time =
		std::chrono::milliseconds( 250 );

	void
	evt_next_turn()
	{
		// Get the next portion.
		auto requests = m_receiver->
				get_one< std::vector< application_request > >()
				.wait_forever()
				.sync_get< a_load_receiver_t::msg_take_requests >();

		if( requests.empty() )
		{
			TRACE() << "PRO(" << m_name << ") no request received, sleeping"
					<< std::endl;
			so_5::send_delayed< msg_next_turn >(
					*this, so_direct_mbox(), next_turn_sleep_time );
		}
		else
		{
			process_requests( requests );
			// Start next turn immediately.
			so_5::send< msg_next_turn >( *this );
		}
	}

	void
	process_requests( const std::vector< application_request > & requests )
	{
		TRACE() << "PRO(" << m_name << ") start processing, requests="
				<< requests.size() << std::endl;

		const auto processing_time = std::chrono::microseconds(
				requests.size() * random( 150, 1500 ) );

		std::this_thread::sleep_for( processing_time );

		TRACE() << "PRO(" << m_name << ") processing took: "
				<< processing_time.count() / 1000.0 << "ms" << std::endl;
	}

	int
	random( int low, int high )
	{
		return std::uniform_int_distribution<>( low, high )( m_random_engine );
	}
};

std::vector< so_5::rt::mbox_ref_t >
create_processing_coops( so_5::rt::environment_t & env )
{
	std::vector< so_5::rt::mbox_ref_t > result;

	std::size_t capacities[] = { 25, 35, 40, 15, 20 };

	int i = 0;
	for( auto c : capacities )
	{
		auto coop = env.create_coop( "processor_" + std::to_string(i) );

		auto receiver = std::unique_ptr< a_load_receiver_t >(
				new a_load_receiver_t( env, "r" + std::to_string(i), c ) );

		auto receiver_mbox = receiver->so_direct_mbox();
		result.push_back( receiver_mbox );

		auto processor = std::unique_ptr< a_processor_t >(
				new a_processor_t( env, "p" + std::to_string(i), receiver_mbox ) );

		coop->add_agent( std::move( receiver ),
				so_5::disp::thread_pool::create_disp_binder(
						"receivers",
						so_5::disp::thread_pool::params_t() ) );

		coop->add_agent( std::move( processor ),
				so_5::disp::active_obj::create_disp_binder( "processors" ) );

		env.register_coop( std::move( coop ) );

		++i;
	}

	return result;
}

void
init( so_5::rt::environment_t & env )
{
	auto receivers = create_processing_coops( env );

	auto coop = env.create_coop( "generators",
			so_5::disp::thread_pool::create_disp_binder(
					"generators",
					so_5::disp::thread_pool::params_t() ) );

	for( int i = 0; i != 3; ++i )
	{
		auto agent = std::unique_ptr< a_generator_t >( 
				new a_generator_t( env, "g" + std::to_string(i), receivers ) );
		coop->add_agent( std::move( agent ) );
	}

	env.register_coop( std::move( coop ) );

	std::this_thread::sleep_for( std::chrono::seconds( 10 ) );
	env.stop();
}

int main()
{
	try
	{
		so_5::launch( init,
				[]( so_5::rt::environment_params_t & params ) {
					params.add_named_dispatcher( "generators",
							so_5::disp::thread_pool::create_disp( 3 ) );
					params.add_named_dispatcher( "receivers",
							so_5::disp::thread_pool::create_disp( 2 ) );
					params.add_named_dispatcher( "processors",
							so_5::disp::active_obj::create_disp() );
				} );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


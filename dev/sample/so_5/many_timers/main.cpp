/*
 * Sample of sending big amount of delayed messages.
 * This sample can also be used as stress test for
 * SObjectizer timers implementation.
 */

#include <iostream>
#include <cstring>
#include <cstdlib>

#include <so_5/all.hpp>

// Configuration for the sample.
struct cfg_t
{
	// Count of messages to be sent.
	unsigned long long m_messages = 5000000;

	// Initial delay for every message.
	std::chrono::milliseconds m_delay = std::chrono::milliseconds{ 100 };

	// Type of timer.
	enum class timer_type_t {
		wheel,
		list,
		heap
	} m_timer_type = { timer_type_t::wheel };
};

// Integer argument parsing helper.
template< class T >
T str_to_value( const char * s )
{
	std::stringstream stream;
	stream << s;
	stream.seekg( 0 );

	T r;
	stream >> r;

	if( !stream || !stream.eof() )
		throw std::invalid_argument(
				std::string( "unable to parse value '" ) + s + "'" );

	return r;
}

// Parse command-line arguments and prepare sample configuration.
cfg_t parse_args( int argc, char ** argv )
{
	cfg_t result;

	for( char ** current = &argv[1], ** last = &argv[argc];
			current != last; ++current )
	{
		if( 0 == std::strcmp( *current, "-h" ) )
		{
			std::cout << "Usage:\n\n"
				"sample.so_5.many_timers [options]\n\n"
				"Where options are:\n"
				"-m <count>       count of delayed messages to be sent\n"
				"-d <millisecons> pause for delayed messages\n"
				"-t <type>        timer type (wheel, list, heap)\n"
				"-h               show this help\n"
				<< std::flush;
			std::exit( 1 );
		}
		else if( 0 == std::strcmp( *current, "-d" ) )
		{
			++current;
			if( current == last )
				throw std::invalid_argument( "-d requires value (milliseconds)" );

			result.m_delay = std::chrono::milliseconds(
					str_to_value< unsigned int >( *current ) );
		}
		else if( 0 == std::strcmp( *current, "-m" ) )
		{
			++current;
			if( current == last )
				throw std::invalid_argument( "-m requires value (message count)" );

			result.m_messages = str_to_value< unsigned long long >( *current );
		}
		else if( 0 == std::strcmp( *current, "-t" ) )
		{
			++current;
			if( current == last )
				throw std::invalid_argument( "-t requires value (timer type)" );
			if( 0 == std::strcmp( *current, "wheel" ) )
				result.m_timer_type = cfg_t::timer_type_t::wheel;
			else if( 0 == std::strcmp( *current, "list" ) )
				result.m_timer_type = cfg_t::timer_type_t::list;
			else if( 0 == std::strcmp( *current, "heap" ) )
				result.m_timer_type = cfg_t::timer_type_t::heap;
			else
				throw std::invalid_argument( "unknown type of timer" );
		}
		else
			throw std::invalid_argument( std::string( "unknown argument: " ) +
					*current );
	}

	return result;
}

void show_cfg( const cfg_t & cfg )
{
	std::string timer_type = "wheel";
	if( cfg.m_timer_type == cfg_t::timer_type_t::list )
		timer_type = "list";
	else if( cfg.m_timer_type == cfg_t::timer_type_t::heap )
		timer_type = "heap";

	std::cout << "timer: " << timer_type
			<< ", messages: " << cfg.m_messages
			<< ", delay: " << cfg.m_delay.count() << "ms"
			<< std::endl;
}

// Timer message.
struct msg_timer : public so_5::signal_t {};

// Agent-receiver.
class a_receiver_t : public so_5::agent_t
{
public :
	a_receiver_t(
		context_t ctx,
		unsigned long long messages )
		:	so_5::agent_t( ctx )
		,	m_messages_to_receive( messages )
	{}

	virtual void so_define_agent() override
	{
		so_subscribe_self().event< msg_timer >( &a_receiver_t::evt_timer );
	}

	void evt_timer()
	{
		++m_messages_received;
		if( m_messages_received == m_messages_to_receive )
			so_deregister_agent_coop_normally();
	}

private :
	const unsigned long long m_messages_to_receive;
	unsigned long long m_messages_received = { 0 };
};

// Agent-sender.
class a_sender_t : public so_5::agent_t
{
public :
	a_sender_t(
		context_t ctx,
		so_5::mbox_t dest_mbox,
		unsigned long long messages_to_send,
		std::chrono::milliseconds delay )
		:	so_5::agent_t( ctx )
		,	m_dest_mbox( std::move( dest_mbox ) )
		,	m_messages_to_send( messages_to_send )
		,	m_delay( delay )
	{}

	virtual void so_evt_start() override
	{
		for( unsigned long long i = 0; i != m_messages_to_send; ++i )
			so_environment().single_timer< msg_timer >( m_dest_mbox, m_delay );
	}

private :
	const so_5::mbox_t m_dest_mbox;

	const unsigned long long m_messages_to_send;

	const std::chrono::milliseconds m_delay;
};

void run_sobjectizer( const cfg_t & cfg )
{
	so_5::launch(
		// Initialization actions.
		[&cfg]( so_5::environment_t & env )
		{
			// Active object dispatcher is necessary.
			env.introduce_coop(
				so_5::disp::active_obj::create_private_disp( env )->binder(),
				[&cfg]( so_5::coop_t & coop ) {
					auto a_receiver = coop.make_agent< a_receiver_t >( cfg.m_messages );

					coop.make_agent< a_sender_t >(
							a_receiver->so_direct_mbox(), cfg.m_messages, cfg.m_delay );
				});
		},
		// Parameter tuning actions.
		[&cfg]( so_5::environment_params_t & params )
		{
			// Appropriate timer thread must be used.
			so_5::timer_thread_factory_t timer = so_5::timer_wheel_factory();
			if( cfg.m_timer_type == cfg_t::timer_type_t::list )
				timer = so_5::timer_list_factory();
			else if( cfg.m_timer_type == cfg_t::timer_type_t::heap )
				timer = so_5::timer_heap_factory();

			params.timer_thread( timer );
		} );
}

int main( int argc, char ** argv )
{
	try
	{
		const auto cfg = parse_args( argc, argv );
		show_cfg( cfg );

		run_sobjectizer( cfg );

		return 0;
	}
	catch( const std::exception & x )
	{
		std::cerr << "Exception caught: " << x.what() << std::endl;
	}

	return 2;
}


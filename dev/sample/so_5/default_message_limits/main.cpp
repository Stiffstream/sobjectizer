/*
 * An example of using default message limits.
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

using namespace std::chrono_literals;

// A base part for every request.
struct request_base
{
	// Request ID.
	int m_id;
};

// Messages for different request types.
struct req_A final : request_base {};
struct req_B final : request_base {};
struct req_C final : request_base {};
struct req_D final : request_base {};
struct req_E final : request_base {};
struct req_F final : request_base {};
struct req_G final : request_base {};
struct req_I final : request_base {};

// Message for logger.
struct log_message
{
	// Text to be logged.
	std::string m_what;
};

// Logger agent.
class a_logger_t final : public so_5::agent_t
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

	void so_define_agent() override
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
		ss << double(
				std::chrono::duration_cast< std::chrono::milliseconds >(
						now - m_started_at ).count()
				) / 1000.0 << "ms";

		return ss.str();
	}
};

// An agent that plays a role of "trash can" for rejected requests.
class a_trash_can_t final : public so_5::agent_t
{
public :
	a_trash_can_t(
		context_t ctx,
		so_5::mbox_t logger )
		:	so_5::agent_t{ ctx
				// Hold no more than 10 instances of any message type.
				+ limit_then_drop< any_unspecified_message >( 10u )
			}
		,	m_logger{ std::move(logger) }
	{}

	void so_define_agent() override
	{
		so_default_state()
			.event( make_event_handler< req_A >( "req_A" ) )
			.event( make_event_handler< req_B >( "req_B" ) )
			.event( make_event_handler< req_C >( "req_C" ) )
			.event( make_event_handler< req_D >( "req_D" ) )
			.event( make_event_handler< req_E >( "req_E" ) )
			.event( make_event_handler< req_F >( "req_F" ) )
			.event( make_event_handler< req_G >( "req_G" ) )
			.event( make_event_handler< req_I >( "req_I" ) )
			;
	}

private :
	const so_5::mbox_t m_logger;

	template< typename Request >
	[[nodiscard]]
	auto
	make_event_handler( std::string name )
	{
		return [this, req_name=std::move(name)]( mhood_t<Request> cmd ) {
			so_5::send< log_message >( m_logger,
					req_name + ": redirected to trash can, id=" +
					std::to_string(cmd->m_id) );
		};
	}
};

// An agent that does "normal" processing of incoming messages.
class a_processor_t final : public so_5::agent_t
{
public :
	struct shutdown final : public so_5::signal_t {};

	a_processor_t(
		context_t ctx,
		so_5::mbox_t logger,
		so_5::mbox_t trash_can )
		:	so_5::agent_t{ ctx
				// Set the personal limits for several of messages.
				+ limit_then_redirect< req_A >(
						10u, [trash_can]() { return trash_can; } )
				+ limit_then_redirect< req_C >(
						8u, [trash_can]() { return trash_can; } )
				+ limit_then_abort< shutdown >( 1u )
				// All other messages will have the same limit and reaction.
				+ limit_then_redirect< any_unspecified_message >(
						4u, [trash_can]() { return trash_can; } )
			}
		,	m_logger( std::move( logger ) )
	{}

	void so_define_agent() override
	{
		so_default_state()
			.event( make_request_handler< req_A >( "req_A", 1ms ) )
			.event( make_request_handler< req_B >( "req_B", 15ms ) )
			.event( make_request_handler< req_C >( "req_C", 2ms ) )
			.event( make_request_handler< req_D >( "req_D", 18ms ) )
			.event( make_request_handler< req_E >( "req_E", 22ms ) )
			.event( make_request_handler< req_F >( "req_F", 19ms ) )
			.event( make_request_handler< req_G >( "req_G", 24ms ) )
			.event( make_request_handler< req_I >( "req_I", 23ms ) )
			.event( [this]( mhood_t< shutdown > ) {
					so_deregister_agent_coop_normally();
				} )
			;
	}

private :
	const so_5::mbox_t m_logger;

	template< typename Request >
	[[nodiscard]]
	auto make_request_handler(
		std::string name,
		std::chrono::milliseconds duration )
	{
		return [this, req_name=std::move(name), duration]
			( mhood_t< Request > cmd )
			{
				so_5::send< log_message >( m_logger,
						"processing request " + req_name + " (" +
						std::to_string( cmd->m_id ) + ") for " +
						std::to_string( duration.count() ) + "ms" );

				// Imitation of some intensive processing.
				std::this_thread::sleep_for(
						std::chrono::milliseconds( duration ) );
			};
	}
};

using sender_t = void(*)( const so_5::mbox_t & to, int id );

template< typename Request >
[[nodiscard]]
sender_t make_sender()
{
	return []( const so_5::mbox_t & to, int id ) {
		so_5::send< Request >( to, id );
	};
}


void init( so_5::environment_t & env )
{
	const so_5::mbox_t processor_mbox = env.introduce_coop(
		[&env]( so_5::coop_t & coop ) {
			// Logger will work on the default dispatcher.
			auto logger = coop.make_agent< a_logger_t >();

			// Trash can will work on its own dispatcher.
			auto trash_can = coop.make_agent_with_binder< a_trash_can_t >(
					so_5::disp::one_thread::make_dispatcher( env ).binder(),
					logger->so_direct_mbox() );

			// Processor will work on its own dispatcher.
			auto processor = coop.make_agent_with_binder< a_processor_t >(
					so_5::disp::one_thread::make_dispatcher( env ).binder(),
					logger->so_direct_mbox(),
					trash_can->so_direct_mbox() );

			return processor->so_direct_mbox();
		});

	// Senders for messages of various types.
	std::array< sender_t, 10 > senders{
		make_sender< req_A >(),
		make_sender< req_B >(),
		make_sender< req_C >(),
		make_sender< req_D >(),
		make_sender< req_E >(),
		make_sender< req_F >(),
		make_sender< req_G >(),
		make_sender< req_I >(),
		make_sender< req_A >(),
		make_sender< req_E >()
	};

	// Shuffle senders.
	{
		std::random_device rd;
		std::mt19937 generator{ rd() };

		std::shuffle( std::begin(senders), std::end(senders), generator );
	}

	// Do several interations of sending messages.
	int id = 0;
	for( int i = 0; i < 15; ++i )
	{
		for( const auto & s : senders )
			s( processor_mbox, id++ );

		std::this_thread::sleep_for( 13ms );
	}

	// Finish the example.
	so_5::send< a_processor_t::shutdown >( processor_mbox );
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


/*
 * An example of using mchain with empty_notificator.
 *
 * There is a producer that tries to send a message to a mchain, but
 * only if the mchain is empty. The emptyness of the mchain is detected
 * by a empty_notificator.
 */

#include <iostream>
#include <chrono>

#include <so_5/all.hpp>

//
// Stuff for logging.
//
using log_msg = std::string;

so_5::mbox_t make_logger( so_5::coop_t & coop )
{
	class logger_actor final : public so_5::agent_t {
	public:
		using so_5::agent_t::agent_t;

		void so_define_agent() override {
			so_subscribe_self().event( [](mhood_t<log_msg> cmd ) {
					std::cout << *cmd << std::endl;
				} );
		}
	};

	// Logger will work on its own working thread.
	auto logger = coop.make_agent_with_binder< logger_actor >(
			so_5::disp::one_thread::make_dispatcher(
					coop.environment() ).binder() );

	return logger->so_direct_mbox();
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
// Implementation of producers.
//

// A request to be sent for processing.
struct request
{
	std::string m_payload;
};

/*
 * Producer agent will send N requests and then closes mchain.
 */
class producer final : public so_5::agent_t
{
	// This signal initiates next send attempt.
	struct send_next final : public so_5::signal_t {};

	// This signal indicates that the mchain is empty.
	struct mchain_is_empty final : public so_5::signal_t {};

public :
	producer( context_t ctx,
		std::string name,
		so_5::mbox_t logger_mbox,
		unsigned int requests )
		:	so_5::agent_t{ ctx }
		,	m_name( std::move(name) )
		,	m_logger_mbox{ std::move(logger_mbox) }
		,	m_attempts_left{ requests }
	{
		// Create the target mchain.
		m_target_mchain = so_environment().create_mchain(
				so_5::make_limited_without_waiting_mchain_params(
						// Just one message inside, there is no need for more.
						1,
						so_5::mchain_props::memory_usage_t::preallocated,
						so_5::mchain_props::overflow_reaction_t::throw_exception)
				.empty_notificator(
					[m = so_direct_mbox()]() {
						// Tell the agent that the mchain is empty now.
						so_5::send< mchain_is_empty >( m );
					} )
			);
	}

	// Get the target mchain.
	[[nodiscard]]
	so_5::mchain_t target_mchain() const
	{
		return m_target_mchain;
	}

	void so_define_agent() override
	{
		so_subscribe_self()
			.event( &producer::evt_send_next )
			.event( &producer::evt_mchain_is_empty )
			;
	}

	void so_evt_start() override
	{
		// Initiate request sending loop.
		so_5::send< send_next >( *this );
	}

private :
	const std::string m_name;

	const so_5::mbox_t m_logger_mbox;

	// How many attempts remains.
	unsigned int m_attempts_left;

	// The target mchain to be used.
	so_5::mchain_t m_target_mchain;

	// Indicator of mchain's emptiness.
	// The mchain is empty at the beginning.
	bool m_target_is_empty = true;

	// An event for next attempt to send another requests.
	void evt_send_next(mhood_t< send_next >)
	{
		if( m_target_is_empty )
		{
			m_logger_mbox <<= ( msg_maker() << m_name
					<< ": sending a message..." );

			// Assume that mchain won't be empty after the sent.
			m_target_is_empty = false;
			so_5::send< request >(
					m_target_mchain,
					m_name + "_request_" + std::to_string( m_attempts_left ) );
		}
		else
		{
			m_logger_mbox <<= ( msg_maker{} << m_name
					<< ": message is not sent because mchain is full" );
		}

		--m_attempts_left;

		if( m_attempts_left )
		{
			// Next try after a timeout.
			so_5::send_delayed< send_next >(
					*this,
					std::chrono::milliseconds{ 50 } );
		}
		else
		{
			// Nothing to do, it's time to close the target mchain.
			so_5::close_retain_content(
					so_5::exceptions_enabled,
					m_target_mchain );

			m_logger_mbox <<= ( msg_maker{} << m_name
					<< ": target mchain is closed" );
		}
	}

	void evt_mchain_is_empty( mhood_t<mchain_is_empty> )
	{
		m_logger_mbox <<= ( msg_maker{} << m_name
				<< ": mchain_is_empty received" );
		m_target_is_empty = true;
	}
};

void run_example()
{
	so_5::mbox_t logger_mbox;
	so_5::mchain_t chain_to_use;

	// Launch SObjectizer without blocking the current thread.
	so_5::wrapped_env_t sobjectizer{
			so_5::wrapped_env_t::wait_init_completion,
			[&logger_mbox, &chain_to_use]( so_5::environment_t & env )
			{
				chain_to_use = env.introduce_coop(
						[&logger_mbox]( so_5::coop_t & coop ) {
							// Logger will work on its own context.
							logger_mbox = make_logger( coop );

							auto * a_producer = coop.make_agent< producer >(
									"Alice",
									logger_mbox,
									5 );
							return a_producer->target_mchain();
						});
			}
		};

	// Loop for reading messages from chain_to_use.
	auto status = so_5::mchain_props::extraction_status_t::no_messages;
	while( so_5::mchain_props::extraction_status_t::chain_closed != status )
	{
		// Take a pause.
		std::this_thread::sleep_for( std::chrono::milliseconds{ 75 } );

		// Try to get a message from the chain.
		status = so_5::receive(
				so_5::from( chain_to_use )
						.handle_all()
						.no_wait_on_empty(),
				[&logger_mbox]( const request & req )
				{
					logger_mbox <<= "Bob: start handling of received request";

					logger_mbox <<= ( msg_maker{} << "Bob: request payload: "
							<< req.m_payload );

					logger_mbox <<= "Bob: finish handling of received request";
				} ).status();
	}

	// SObjectizer will be shutdown automatically.
}

int main()
{
	try
	{
		run_example();

		return 0;
	}
	catch( const std::exception & x )
	{
		std::cerr << "Exception: " << x.what() << std::endl;
	}

	return 2;
}


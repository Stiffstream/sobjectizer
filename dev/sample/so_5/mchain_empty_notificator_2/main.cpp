/*
 * An example of using mchain with empty_notificator.
 *
 * There is a producer that tries to send a message to a mchain, but
 * only if the mchain is empty. The emptyness of the mchain is detected
 * by a empty_notificator.
 *
 * The producer sends messages to the target mchain until it becomes full
 * and then wait for the emptyness of the mchain to resume sending.
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
						// Use mchain as a buffer of fixed capacity.
						5,
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
		this >>= st_sending_enabled;

		st_sending_enabled
			.event( &producer::evt_send_next )
			;

		st_sending_blocked
			.event( &producer::evt_send_next_when_blocked )
			.event( &producer::evt_mchain_is_empty )
			;
	}

	void so_evt_start() override
	{
		// Initiate request sending loop.
		m_send_timer = so_5::send_periodic< send_next >(
				*this,
				// No pause before the first sent.
				std::chrono::milliseconds::zero(),
				// Pause before next send.
				std::chrono::milliseconds{ 20 });
	}

private :
	// State in that agent can try to send messages.
	state_t st_sending_enabled{ this, "sending_enabled" };
	// State in that agent can't send messages because mchain
	// isn't empty yet.
	state_t st_sending_blocked{ this, "sending_blocked" };

	const std::string m_name;

	const so_5::mbox_t m_logger_mbox;

	// How many attempts remains.
	unsigned int m_attempts_left;

	// The target mchain to be used.
	so_5::mchain_t m_target_mchain;

	// Timer ID for periodic send_next signal.
	so_5::timer_id_t m_send_timer;

	// An event for the next attempt to send another request.
	void evt_send_next(mhood_t< send_next >)
	{
		const auto result = so_5::select(
				so_5::from_all().handle_n( 1 ).no_wait_on_empty(),
				so_5::send_case(
						m_target_mchain,
						so_5::message_holder_t< request >::make(
							m_name + "_request_" + std::to_string( m_attempts_left ) ),
						[this]() {
							m_logger_mbox <<= ( msg_maker() << m_name
									<< ": message stored to target mbox" );
							--m_attempts_left;
						} ) );

		if( !result.was_sent() )
		{
			// Message wasn't sent.
			m_logger_mbox <<= ( msg_maker{} << m_name
					<< ": message is not sent because mchain is full" );

			// The sending has to be paused.
			this >>= st_sending_blocked;
		}
		else
		{
			if( !m_attempts_left )
			{
				// It's time to close the target mchain.
				so_5::close_retain_content(
						so_5::exceptions_enabled,
						m_target_mchain );

				// Periodic message has to be cancelled because all
				// messages have been sent.
				m_send_timer.release();
			}
		}
	}

	// An event for next attempt to send another requests.
	void evt_send_next_when_blocked(mhood_t< send_next >)
	{
		m_logger_mbox <<= ( msg_maker{} << m_name
				<< ": message can't be sent because mchain is full" );
	}

	// Reaction to notification about emptiness of the target mbox.
	void evt_mchain_is_empty( mhood_t<mchain_is_empty> )
	{
		m_logger_mbox <<= ( msg_maker{} << m_name
				<< ": mchain_is_empty received" );
		this >>= st_sending_enabled;
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
									20 );
							return a_producer->target_mchain();
						});
			}
		};

	// Loop for reading messages from chain_to_use.
	auto status = so_5::mchain_props::extraction_status_t::no_messages;
	while( so_5::mchain_props::extraction_status_t::chain_closed != status )
	{
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

					// Take a pause.
					std::this_thread::sleep_for( std::chrono::milliseconds{ 75 } );

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


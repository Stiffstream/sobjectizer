/*
 * A sample of the simpliest agent with so_evt_async_start.
 */

#include <iostream>

// Main SObjectizer header files.
#include <so_5/all.hpp>

// Definition of an agent for SObjectizer.
class a_hello final : public so_5::agent_t
{
	// Type of a signal to be sent to mchain.
	struct msg_hello final : public so_5::signal_t {};

	// Type of a signal to be sent to the agent directly.
	struct msg_bye final : public so_5::signal_t {};

	// Destination for the delayed signal.
	so_5::mchain_t m_ch;

public:
	a_hello( context_t ctx )
		: so_5::agent_t{ ctx
			// Support for C++20 coroutines has to be enabled explicitly.
			+ so_5::enable_cpp_coro() }
		, m_ch{ so_5::create_mchain( so_environment() ) }
	{}

	// Tunning of the agent.
	void so_define_agent() override
	{
		// msg_bye has to be received by a subscription.
		so_subscribe_self().event( &a_hello::evt_bye );
	}

	// A reaction to start of work in SObjectizer.
	void so_evt_start() override
	{
		std::cout << "synchronous start" << std::endl;

		// msg_hello is sent as delayed message.
		so_5::send_delayed< msg_hello >( m_ch, std::chrono::milliseconds{ 100 } );

		// msg_bye is sent as ordinary message with immediately delivery.
		so_5::send< msg_bye >( *this );
	}

	// An asynchronous reaction to start of work in SObjectizer.
	so_5::cpp_coro::task_t< void > so_evt_async_start() override
	{
		std::cout << "asynchronous start" << std::endl;

		// Wait arrival of msg_hello.
		co_await so_5::cpp_coro::select(
				so_5::from_all().handle_n( 1 ),
				so_5::receive_case( m_ch,
					[]( mhood_t< msg_hello > ) {
						std::cout << "msg_hello received" << std::endl;
					} ) );

		// The cooperation with this agent can now be deregistered.
		so_deregister_agent_coop_normally();
	}

	// A reaction to finish of work in SObjectizer.
	void so_evt_finish() override
	{
		std::cout << "synchronous finish" << std::endl;
	}

private:
	void evt_bye( mhood_t< msg_bye > )
	{
		std::cout << "msg_bye received" << std::endl;
	}
};

int main()
{
	try
	{
		// Starting SObjectizer.
		so_5::launch(
			// A function for SO Environment initialization.
			[]( so_5::environment_t & env )
			{
				// Creating and registering single agent as a cooperation.
				env.register_agent_as_coop( env.make_agent< a_hello >() );
			} );
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}


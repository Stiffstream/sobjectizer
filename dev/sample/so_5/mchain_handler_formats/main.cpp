/*
 * An example for demonstration of different format of message handlers for mchain.
 */

#include <iostream>
#include <string>

// Main SObjectizer header file.
#include <so_5/all.hpp>

// Types of messages and signals to be placed into mchain.
struct two_ints
{
	int a, b;
};

struct delayed_two_ints
{
	int a, b;
};

struct first_signal : public so_5::signal_t {};
struct second_signal : public so_5::signal_t {};
struct periodic_signal : public so_5::signal_t {};

void demo()
{
	// A SObjectizer instance.
	so_5::wrapped_env_t sobj;

	// Message chain to be used.
	auto ch = so_5::create_mchain( sobj,
			// Wait on overloaded mchain for 5min.
			std::chrono::minutes{5},
			// No more than 3 messages in chain.
			3u,
			// Space for mchain will be preallocated.
			so_5::mchain_props::memory_usage_t::preallocated,
			// What to do on overflow.
			// This value has no sence because we use too large time limit.
			// Because of that use hardest case.
			so_5::mchain_props::overflow_reaction_t::abort_app );

	// We must store timer_id for periodic signal somewhere.
	// Otherwise periodic signal will be canceled automaticallly right after
	// exit from send_periodic function.
	so_5::timer_id_t periodic_signal_timer;

	// Spawn another thread to fill mchain.
	std::thread worker{ [&]{
		// Send a string as message.
		so_5::send< std::string >( ch, "Hello!" );
		// Send int as message.
		so_5::send< int >( ch, 42 );
		// Send struct instance as message.
		so_5::send< two_ints >( ch, 0, 1 );
		// Send delayed messages.
		so_5::send_delayed< delayed_two_ints >(
				ch, std::chrono::milliseconds{150}, 1, 2 );
		// Send signals.
		so_5::send< first_signal >( ch );
		so_5::send< second_signal >( ch );
		// Send periodic signal.
		periodic_signal_timer = so_5::send_periodic< periodic_signal >(
				ch, std::chrono::milliseconds{20}, std::chrono::milliseconds{150} );
	} };

	// Read content of mchain.
	// We expect exactly 8 messages/signals.
	receive( from( ch ).handle_n( 8 ),
		// Message instance by const reference.
		[]( const std::string & v ) { std::cout << "str: " << v << std::endl; },
		// Message instance by value (efficient for small types like int).
		[]( int v ) { std::cout << "int: " << v << std::endl; },
		// Message instance via mhood_t value.
		[]( so_5::mhood_t< two_ints > v ) {
			std::cout << "two_ints: " << v->a << ", " << v->b << std::endl;
		},
		// Message instance via const reference to mhood_t.
		[]( const so_5::mhood_t< delayed_two_ints > & v ) {
			std::cout << "delayed_two_ints: " << v->a << ", " << v->b << std::endl;
		},
		// Explicitly specified signal handler.
		so_5::handler< first_signal >( []{ std::cout << "first signal" << std::endl; } ),
		// Signal handler via mhood_t value.
		[]( so_5::mhood_t< second_signal > ) {
			std::cout << "second signal" << std::endl;
		},
		// Signal handler via const reference to mhood_t.
		[]( const so_5::mhood_t< periodic_signal > & ) {
			std::cout << "periodic signal" << std::endl;
		} );

	// Close channel with dropping all its content.
	// It has sence because timer thread can send periodic signals to
	// mchain while we wait on worker.join().
	close_drop_content( ch );

	worker.join();

	// SObjectizer will be stopped automatically.
}

int main()
{
	try
	{
		demo();
	}
	catch( const std::exception & ex )
	{
		std::cerr << "Error: " << ex.what() << std::endl;
	}

	return 0;
}


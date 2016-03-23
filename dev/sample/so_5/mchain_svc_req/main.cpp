/*
 * An example of sending service requests to mchain.
 */

#include <iostream>
#include <string>

// Main SObjectizer header file.
#include <so_5/all.hpp>

void demo()
{
	// A SObjectizer instance.
	so_5::wrapped_env_t sobj;

	// Message chain to be used.
	// Chain will have unlimited dynamic message queue.
	auto ch = create_mchain( sobj );

	// Create an agent for sending service requests.
	sobj.environment().introduce_coop( [&ch]( so_5::coop_t & coop ) {
			auto a = coop.define_agent();
			a.on_start( [a] {
					// Start service requests sending loop by sending
					// the first value to itself.
					so_5::send< int >( a, 10 );
				} );
			a.event( a, [a, &ch]( int v ) {
					if( v > 0 ) {
						// Loop must be continued.
						// Requesting translation of value.
						auto converted = so_5::request_value< std::string, int >(
								ch, so_5::infinite_wait, v );
						std::cout << v << " -> '" << converted << "'" << std::endl;

						// Continue loop by sending next value to itself.
						so_5::send< int >( a, v - 1 );
					}
					else
						// Finishing the loop by closing chain.
						close_drop_content( ch );
				} );
		} );

	// Handling of requests from SObjectizer's agent on the current thread.
	// The 'receive' will return when the chain will be closed.
	receive( from(ch), []( int v ) { return std::to_string(v); } );

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


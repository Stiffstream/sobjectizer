/*
 * An example of sending service requests to mchain.
 */

// Main SObjectizer header file.
#include <so_5/all.hpp>

// Helper agent for sending requests to mchain.
class request_sender_t final : public so_5::agent_t
{
public :
	request_sender_t( context_t ctx, const so_5::mchain_t & ch )
		:	so_5::agent_t{ std::move(ctx) }
	{
		so_subscribe_self().event( [this, ch](mhood_t<int> cmd ) {
				if( *cmd > 0 ) {
					// Loop must be continued.
					// Requesting translation of value.
					auto converted = so_5::request_value< std::string, int >(
							ch, so_5::infinite_wait, *cmd );
					std::cout << *cmd << " -> '" << converted << "'" << std::endl;

					// Continue loop by sending next value to itself.
					so_5::send< int >( *this, *cmd - 1 );
				}
				else
					// Finishing the loop by closing chain.
					close_drop_content( ch );
			} );
	}

	void so_evt_start() override
	{
		// Start service requests sending loop by sending
		// the first value to itself.
		so_5::send< int >( *this, 10 );
	}
};

void demo()
{
	// A SObjectizer instance.
	so_5::wrapped_env_t sobj;

	// Message chain to be used.
	// Chain will have unlimited dynamic message queue.
	auto ch = create_mchain( sobj );

	// Create an agent for sending service requests.
	sobj.environment().introduce_coop( [&ch]( so_5::coop_t & coop ) {
			coop.make_agent< request_sender_t >( std::cref(ch) );
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


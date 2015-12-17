/*
 * Example for demonstration of usage of PImpl idiom for implementation
 * of agents.
 */

#include <iostream>

#include <so_5/all.hpp>

#include "key_value_storage.hpp"

// Main agent of the example.
//
// Creates several key-value pairs at the start of its works.
// Then periodically request for each of them.
//
// Due to expiration of key-value lifetimes the agents should receive
// negative responses for its requests from time to time.
//
class a_consumer_t : public so_5::agent_t
{
public :
	a_consumer_t(
		// Environment to work in.
		context_t ctx,
		// Mbox of key-value-storage.
		so_5::mbox_t storage_mbox )
		:	so_5::agent_t( ctx )
		,	m_storage_mbox( std::move( storage_mbox ) )
	{
		m_values.emplace_back( std::make_pair( "first", "value for first" ) );
		m_values.emplace_back( std::make_pair( "second", "value for second" ) );
		m_values.emplace_back( std::make_pair( "third", "value for third" ) );
		m_values.emplace_back( std::make_pair( "forth", "value for forth" ) );
	}

	virtual void so_define_agent() override
	{
		// Just one signal must be handled by main agent.
		so_default_state().event< msg_next_turn >( &a_consumer_t::evt_next_turn );
	}

	virtual void so_evt_start() override
	{
		// Do registration of key-value pairs.
		// Lifetime will be incremented for next pair.
		std::chrono::milliseconds lifetime{ 100 };

		for( const auto & kv : m_values )
		{
			so_5::send< msg_register_pair >( m_storage_mbox,
					kv.first,
					kv.second,
					lifetime );
			lifetime += std::chrono::milliseconds{ 75 }; 
		}

		// Initiate loop of requests for the values.
		so_5::send< msg_next_turn >( *this );
	}

private :
	// Signal for next interation of requests loop.
	struct msg_next_turn : public so_5::signal_t {};

	// Mbox of key-value-storage.
	const so_5::mbox_t m_storage_mbox;

	// Values to be placed into storage.
	std::vector< std::pair< std::string, std::string > > m_values;

	// Next interation of requests loop.
	void evt_next_turn()
	{
		std::cout << "--- next iteration ---" << std::endl;

		std::size_t values_found = 0;

		for( const auto & kv : m_values )
		{
			try
			{
				// Will use sync-requests for requests of value.
				auto result = so_5::request_value< std::string, msg_request_by_key >(
						m_storage_mbox, so_5::infinite_wait, kv.first );

				std::cout << "key: '" << kv.first << "', expected: '"
						<< kv.second << "', received: '" << result << "'"
						<< std::endl;

				values_found += 1;
			}
			catch( const key_not_found_exception & )
			{
				std::cout << "value '" << kv.first << "' IS NOT FOUND"
						<< std::endl;
			}
		}

		if( values_found )
			// Loop count be continued.
			so_5::send_delayed< msg_next_turn >( *this,
					std::chrono::milliseconds{ 50 } );
		else
			// Work can be finished. All values removed due to
			// lifetime expiration.
			so_deregister_agent_coop_normally();
	}
};

int main()
{
	try
	{
		so_5::launch( []( so_5::environment_t & env )
			{
				// All agents of example must be active agents.
				env.introduce_coop(
					// Agents will be bound to private active_obj dispatcher.
					so_5::disp::active_obj::create_private_disp( env )->binder(),
					[]( so_5::coop_t & coop ) {
						auto storage = coop.make_agent< a_key_value_storage_t >();

						coop.make_agent< a_consumer_t >( storage->so_direct_mbox() );
					});
			} );

		return 0;
	}
	catch( const std::exception & x )
	{
		std::cerr << "Exception: " << x.what() << std::endl;
	}

	return 2;
}


#include "key_value_storage.hpp"

#include <map>

// Declaration of key-value-storage actual implementation.
struct a_key_value_storage_t::internals_t
{
	// Type of delayed message to be used for removing
	// key-value pair after expiration of its lifetime.
	struct msg_lifetime_expired
	{
		std::string m_key;
	};

	// Type of storage for key-value pairs.
	using values_map_t = std::map< std::string, std::string >;

	// Storage for key-value pairs.
	values_map_t m_values;

	// Registration of new pair in the storage.
	void register_pair(
		a_key_value_storage_t & self,
		const msg_register_pair & what )
	{
		auto r = m_values.emplace(
				values_map_t::value_type{ what.m_key, what.m_value } );
		if( r.second )
			// New value really inserted.
			// Lifetime for it must be controlled.
			so_5::send_delayed< msg_lifetime_expired >(
					self,
					what.m_lifetime,
					what.m_key );
	}

	// Handle request for the pair.
	// Throws an exception if pair is not found.
	std::string handle_request_for_pair(
		const msg_request_by_key & what )
	{
		auto r = m_values.find( what.m_key );
		if( r == m_values.end() )
			throw key_not_found_exception( "key is not found in the storage: " +
					what.m_key );

		return r->second;
	}

	// Handle lifetime expiration for the pair.
	void handle_lifetime_expiration(
		const msg_lifetime_expired & evt )
	{
		m_values.erase( evt.m_key );
	}
};

a_key_value_storage_t::a_key_value_storage_t( context_t ctx )
	:	so_5::agent_t( ctx )
	,	m_impl( new internals_t() )
{}
a_key_value_storage_t::~a_key_value_storage_t()
{}

void
a_key_value_storage_t::so_define_agent()
{
	// All events will be handled in the default state.
	//
	// All events are implemented by lambda-functions.
	// This allows to hide the actual implementation of
	// agent's internals from agent's users.
	//
	so_default_state()
		.event( [this]( const msg_register_pair & evt ) {
					m_impl->register_pair( *this, evt );
				} )
		.event( [this]( const msg_request_by_key & evt ) {
					return m_impl->handle_request_for_pair( evt );
				} )
		.event( [this]( const internals_t::msg_lifetime_expired & evt ) {
					m_impl->handle_lifetime_expiration( evt );
				} );
}


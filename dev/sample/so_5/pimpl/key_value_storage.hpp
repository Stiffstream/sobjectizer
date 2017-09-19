/*
 * Interface of key-value-storage agent.
 */

#pragma once

#include <string>
#include <chrono>
#include <stdexcept>

#include <so_5/all.hpp>

// Message for registration key-value pair in the storage.
struct msg_register_pair
{
	std::string m_key;
	std::string m_value;

	// Lifetime for the pair.
	// After expiration of that lifetime pair will be automatically removed.
	std::chrono::milliseconds m_lifetime;
};

// A request for the value by key.
struct msg_request_by_key
{
	std::string m_key;
};

// An exception to be thrown if key is not found in the storage.
class key_not_found_exception : public std::logic_error
{
public :
	key_not_found_exception( const std::string & what )
		:	std::logic_error( what )
	{}
};

// An agent for implementing key-value-storage.
// The real implementation is hidden by PImpl idiom.
class a_key_value_storage_t : public so_5::agent_t
{
public :
	a_key_value_storage_t( context_t ctx );
	~a_key_value_storage_t() override;

	virtual void so_define_agent() override;

private :
	struct internals_t;

	// The real implementation of agent.
	std::unique_ptr< internals_t > m_impl;
};


/*
	SObjectizer 5.
*/

#include <so_5/rt/h/message.hpp>

namespace so_5
{

//
// message_t
//

message_t::message_t()
	:	m_mutability( message_mutability_t::immutable_message )
{
}

message_t::message_t( const message_t & other )
	:	atomic_refcounted_t()
	,	m_mutability( other.m_mutability )
{
}

message_t::message_t( message_t && other )
	:	atomic_refcounted_t()
	,	m_mutability( other.m_mutability )
{
}

message_t &
message_t::operator=( const message_t & other )
{
	m_mutability = other.m_mutability;
	return *this;
}

message_t &
message_t::operator=( message_t && other )
{
	m_mutability = other.m_mutability;
	return *this;
}

const void *
message_t::so5__payload_ptr() const
{
	return this;
}

} /* namespace so_5 */


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
{
}

message_t::message_t( const message_t & )
	:	atomic_refcounted_t()
{
}

message_t::~message_t()
{
}

void
message_t::operator = ( const message_t & )
{
}

const void *
message_t::so5__payload_ptr() const
{
	return this;
}

//
// signal_t
//

signal_t::signal_t()
{}

signal_t::~signal_t()
{}

} /* namespace so_5 */


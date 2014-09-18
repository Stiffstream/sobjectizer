/*
	SObjectizer 5.
*/

#include <so_5/rt/h/mbox.hpp>

namespace so_5
{

namespace rt
{

mbox_t::mbox_t()
{
}

mbox_t::~mbox_t()
{
}

bool
mbox_t::operator==( const mbox_t & o ) const
{
	return id() == o.id();
}

bool
mbox_t::operator<( const mbox_t & o ) const
{
	return id() < o.id();
}

} /* namespace rt */

} /* namespace so_5 */

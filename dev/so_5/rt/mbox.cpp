/*
	SObjectizer 5.
*/

#include <so_5/rt/h/mbox.hpp>

namespace so_5
{

//
// delivery_filter_t
//
delivery_filter_t::delivery_filter_t()
	{}

delivery_filter_t::~delivery_filter_t()
	{}

//
// abstract_message_box_t
//
abstract_message_box_t::abstract_message_box_t()
{
}

abstract_message_box_t::~abstract_message_box_t()
{
}

bool
abstract_message_box_t::operator==( const abstract_message_box_t & o ) const
{
	return id() == o.id();
}

bool
abstract_message_box_t::operator<( const abstract_message_box_t & o ) const
{
	return id() < o.id();
}

} /* namespace so_5 */

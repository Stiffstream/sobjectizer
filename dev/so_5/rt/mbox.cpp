/*
	SObjectizer 5.
*/

#include <so_5/rt/h/mbox.hpp>

namespace so_5
{

//
// abstract_message_box_t
//

void
abstract_message_box_t::do_deliver_message_from_timer(
	const std::type_index & msg_type,
	const message_ref_t & message )
{
	this->do_deliver_message( msg_type, message, 1 );
}

} /* namespace so_5 */


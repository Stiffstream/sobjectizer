/*
	SObjectizer 5.
*/

#include <so_5/rt/h/mbox.hpp>

#include <so_5/h/ret_code.hpp>

namespace so_5
{

//
// abstract_message_box_t
//

void
abstract_message_box_t::do_deliver_enveloped_msg(
	const std::type_index & /*msg_type*/,
	const message_ref_t & /*message*/,
	unsigned int /*overlimit_reaction_deep*/ )
{
	SO_5_THROW_EXCEPTION( rc_not_implemented,
			"do_deliver_enveloped_msg is not implemented by default" );
}

void
abstract_message_box_t::do_deliver_message_from_timer(
	const std::type_index & msg_type,
	const message_ref_t & message )
{
	const auto kind = message_kind( message );
	if( message_t::kind_t::enveloped_msg == kind )
		this->do_deliver_enveloped_msg( msg_type, message, 1 );
	else
		this->do_deliver_message( msg_type, message, 1 );
}

} /* namespace so_5 */


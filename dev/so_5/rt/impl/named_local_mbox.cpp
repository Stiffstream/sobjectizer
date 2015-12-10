/*
	SObjectizer 5.
*/

#include <algorithm>

#include <so_5/rt/impl/h/named_local_mbox.hpp>

namespace so_5
{

namespace impl
{

//
// named_local_mbox_t
//

named_local_mbox_t::named_local_mbox_t(
	const std::string & name,
	const mbox_t & mbox,
	impl::mbox_core_t & mbox_core )
	:
		m_name( name ),
		m_mbox_core( &mbox_core ),
		m_mbox( mbox )
{
}

named_local_mbox_t::~named_local_mbox_t()
{
	m_mbox_core->destroy_mbox( m_name );
}

mbox_id_t
named_local_mbox_t::id() const
{
	return m_mbox->id();
}

void
named_local_mbox_t::subscribe_event_handler(
	const std::type_index & type_wrapper,
	const so_5::message_limit::control_block_t * limit,
	agent_t * subscriber )
{
	m_mbox->subscribe_event_handler( type_wrapper, limit, subscriber );
}

void
named_local_mbox_t::unsubscribe_event_handlers(
	const std::type_index & type_wrapper,
	agent_t * subscriber )
{
	return m_mbox->unsubscribe_event_handlers( type_wrapper, subscriber );
}

std::string
named_local_mbox_t::query_name() const
{
	return m_name;
}

mbox_type_t
named_local_mbox_t::type() const
{
	return m_mbox->type();
}

void
named_local_mbox_t::do_deliver_message(
	const std::type_index & msg_type,
	const message_ref_t & message,
	unsigned int overlimit_reaction_deep ) const
{
	m_mbox->do_deliver_message( msg_type, message, overlimit_reaction_deep );
}

void
named_local_mbox_t::do_deliver_service_request(
	const std::type_index & msg_type,
	const message_ref_t & message,
	unsigned int overlimit_reaction_deep ) const
{
	m_mbox->do_deliver_service_request(
			msg_type, message, overlimit_reaction_deep );
}

void
named_local_mbox_t::set_delivery_filter(
	const std::type_index & msg_type,
	const delivery_filter_t & filter,
	agent_t & subscriber )
{
	m_mbox->set_delivery_filter( msg_type, filter, subscriber );
}

void
named_local_mbox_t::drop_delivery_filter(
	const std::type_index & msg_type,
	agent_t & subscriber ) SO_5_NOEXCEPT
{
	m_mbox->drop_delivery_filter( msg_type, subscriber );
}

} /* namespace impl */

} /* namespace so_5 */


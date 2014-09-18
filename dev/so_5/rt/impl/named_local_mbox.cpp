/*
	SObjectizer 5.
*/

#include <algorithm>

#include <so_5/rt/impl/h/named_local_mbox.hpp>

namespace so_5
{

namespace rt
{

namespace impl
{

//
// named_local_mbox_t
//

named_local_mbox_t::named_local_mbox_t(
	const std::string & name,
	const mbox_ref_t & mbox,
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
	agent_t * subscriber )
{
	m_mbox->subscribe_event_handler( type_wrapper, subscriber );
}

void
named_local_mbox_t::unsubscribe_event_handlers(
	const std::type_index & type_wrapper,
	agent_t * subscriber )
{
	return m_mbox->unsubscribe_event_handlers( type_wrapper, subscriber );
}

void
named_local_mbox_t::deliver_message(
	const std::type_index & type_wrapper,
	const message_ref_t & message_ref ) const
{
	m_mbox->deliver_message( type_wrapper, message_ref );
}

void
named_local_mbox_t::deliver_service_request(
	const std::type_index & type_index,
	const message_ref_t & svc_request_ref ) const
{
	m_mbox->deliver_service_request( type_index, svc_request_ref );
}

std::string
named_local_mbox_t::query_name() const
{
	return m_name;
}

} /* namespace impl */

} /* namespace rt */

} /* namespace so_5 */

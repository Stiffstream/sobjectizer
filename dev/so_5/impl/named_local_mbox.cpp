/*
	SObjectizer 5.
*/

#include <algorithm>

#include <so_5/impl/named_local_mbox.hpp>

namespace so_5
{

namespace impl
{

//
// named_local_mbox_t
//

named_local_mbox_t::named_local_mbox_t(
	full_named_mbox_id_t name,
	const mbox_t & mbox,
	impl::mbox_core_t & mbox_core )
	:
		m_name( std::move(name) ),
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
	abstract_message_sink_t & subscriber )
{
	m_mbox->subscribe_event_handler( type_wrapper, subscriber );
}

void
named_local_mbox_t::unsubscribe_event_handler(
	const std::type_index & type_wrapper,
	abstract_message_sink_t & subscriber ) noexcept
{
	return m_mbox->unsubscribe_event_handler( type_wrapper, subscriber );
}

std::string
named_local_mbox_t::query_name() const
{
	if( default_global_mbox_namespace() == m_name.m_namespace )
		return m_name.m_name;
	else
		return m_name.m_namespace + "::" + m_name.m_name;
}

mbox_type_t
named_local_mbox_t::type() const
{
	return m_mbox->type();
}

void
named_local_mbox_t::do_deliver_message(
	message_delivery_mode_t delivery_mode,
	const std::type_index & msg_type,
	const message_ref_t & message,
	unsigned int redirection_deep )
{
	m_mbox->do_deliver_message(
			delivery_mode,
			msg_type,
			message,
			redirection_deep );
}

void
named_local_mbox_t::set_delivery_filter(
	const std::type_index & msg_type,
	const delivery_filter_t & filter,
	abstract_message_sink_t & subscriber )
{
	m_mbox->set_delivery_filter( msg_type, filter, subscriber );
}

void
named_local_mbox_t::drop_delivery_filter(
	const std::type_index & msg_type,
	abstract_message_sink_t & subscriber ) noexcept
{
	m_mbox->drop_delivery_filter( msg_type, subscriber );
}

environment_t &
named_local_mbox_t::environment() const noexcept
{
	return m_mbox->environment();
}

} /* namespace impl */

} /* namespace so_5 */


/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Stuff related to individual message delivery tracing.
 *
 * \since v.5.8.3
 */

#include <so_5/msg_tracing_individual.hpp>

#include <so_5/exception.hpp>

namespace so_5::msg_tracing
{

namespace impl
{

//
// special_enveloping_mbox_t
//
special_enveloping_mbox_t::special_enveloping_mbox_t(
	mbox_t dest )
	:	m_dest{ std::move(dest) }
	{}

mbox_id_t
special_enveloping_mbox_t::id() const
	{
		return m_dest->id();
	}

void
special_enveloping_mbox_t::subscribe_event_handler(
	const std::type_index & /*type_index*/,
	abstract_message_sink_t & /*subscriber*/ )
	{
		SO_5_THROW_EXCEPTION( rc_not_implemented,
				"subscribe_event_handler is not supported for "
				"msg_tracing::impl::special_enveloping_mbox_t" );
	}

void
special_enveloping_mbox_t::unsubscribe_event_handler(
	const std::type_index & /*type_index*/,
	abstract_message_sink_t & /*subscriber*/ ) noexcept
	{
		// Nothing to do.
	}

std::string
special_enveloping_mbox_t::query_name() const
	{
		return m_dest->query_name();
	}

mbox_type_t
special_enveloping_mbox_t::type() const
	{
		return m_dest->type();
	}

void
special_enveloping_mbox_t::do_deliver_message(
	message_delivery_mode_t delivery_mode,
	const std::type_index & msg_type,
	const message_ref_t & message,
	unsigned int redirection_deep )
	{
		// The message has to be wrapped into a special envelope.
		message_ref_t wrapped_msg{
				std::make_unique< individual_tracing_envelope_t >( message )
			};

		m_dest->do_deliver_message( delivery_mode, msg_type, wrapped_msg, redirection_deep );
	}

void
special_enveloping_mbox_t::set_delivery_filter(
	const std::type_index & /*msg_type*/,
	const delivery_filter_t & /*filter*/,
	abstract_message_sink_t & /*subscriber*/ )
	{
		SO_5_THROW_EXCEPTION( rc_not_implemented,
				"set_delivery_filter is not supported for "
				"msg_tracing::impl::special_enveloping_mbox_t" );
	}

void
special_enveloping_mbox_t::drop_delivery_filter(
	const std::type_index & /*msg_type*/,
	abstract_message_sink_t & /*subscriber*/ ) noexcept
	{
		// Nothing to do.
	}

so_5::environment_t &
special_enveloping_mbox_t::environment() const noexcept
	{
		return m_dest->environment();
	}

//
// individual_tracing_envelope_t
//
individual_tracing_envelope_t::individual_tracing_envelope_t(
	message_ref_t message )
	:	m_message{ std::move(message) }
	{}

void
individual_tracing_envelope_t::access_hook(
	access_context_t /*context*/,
	handler_invoker_t & invoker ) noexcept
	{
		invoker.invoke( payload_info_t{ m_message } );
	}

} /* namespace impl */

//
// make_individual_trace_filter
//
[[nodiscard]] SO_5_FUNC
filter_shptr_t
make_individual_trace_filter()
	{
		return make_filter( []( const trace_data_t & td ) -> bool {
				const auto instance_info = td.message_instance_info();
				if( instance_info && instance_info->m_envelope )
				{
					// Use the fact that instance_info.m_envelope is a pointer
					// to message_t.
					const message_t * msg =
							//FIXME: it's better to have m_envelope as message_t pointer.
							static_cast<const message_t *>(instance_info->m_envelope);

					// It has to be an instance of the individual_tracing_envelope_t.
					if( dynamic_cast< const impl::individual_tracing_envelope_t * >(msg) )
						// Only now we can enable tracing.
						return true;
				}

				// In all other cases the tracing has to be suppressed.
				return false;
			} );
	}

} /* namespace so_5::msg_tracing */


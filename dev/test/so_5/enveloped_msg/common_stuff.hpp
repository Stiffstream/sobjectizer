/*
 * Various common stuff for testing enveloped messages.
 */

#pragma once

#include <so_5/all.hpp>
#include <so_5/h/stdcpp.hpp>

class trace_t final
	{
		std::mutex m_lock;
		std::string m_content;

	public:
		void
		append( const std::string & what )
			{
				std::lock_guard< std::mutex > lock{ m_lock };
				m_content += what;
			}

		std::string
		content()
			{
				std::string result;
				{
					std::lock_guard< std::mutex > lock{ m_lock };
					result = m_content;
				}
				return result;
			}
	};

class test_envelope_t : public so_5::enveloped_msg::envelope_t
{
	so_5::outliving_reference_t< trace_t > m_trace;
	const std::string m_id;

	so_5::message_ref_t m_payload;

	void
	append_text( const char * what )
	{
		m_trace.get().append( m_id + ":" + what );
	}

public:
	test_envelope_t(
		so_5::outliving_reference_t< trace_t > trace,
		std::string id,
		so_5::message_ref_t payload )
		:	m_trace{ trace }
		,	m_id( std::move(id) )
		,	m_payload{ std::move(payload) }
		{}

	void
	access_hook(
		access_context_t context,
		handler_invoker_t & invoker ) SO_5_NOEXCEPT override
	{
		switch( context )
		{
		case access_context_t::handler_found:
			append_text( "pre_invoke;" );
			invoker.invoke( payload_info_t{ m_payload } );
			append_text( "post_invoke;" );
		break;

		case access_context_t::transformation:
			append_text( "transform;" );
			invoker.invoke( payload_info_t{ m_payload } );
		break;

		case access_context_t::inspection:
			append_text( "inspect;" );
			invoker.invoke( payload_info_t{ m_payload } );
		break;
		}
	}
};

template< class Envelope = test_envelope_t >
class special_mbox_t : public so_5::abstract_message_box_t
{
	const so_5::mbox_t m_actual_mbox;
	so_5::outliving_reference_t< trace_t > m_trace;

	const std::string m_id;

	mutable std::mutex m_counter_lock;
	mutable int m_counter{};

	std::string
	allocate_counter() const
	{
		std::lock_guard< std::mutex > lock{ m_counter_lock };
		return m_id + "[" + std::to_string(++m_counter) + "]";
	}

public :
	special_mbox_t(
		so_5::mbox_t actual_mbox,
		so_5::outliving_reference_t< trace_t > trace,
		std::string id )
		:	m_actual_mbox{ std::move(actual_mbox) }
		,	m_trace{ trace }
		,	m_id( std::move(id) )
	{}

	static so_5::mbox_t
	make(
		so_5::mbox_t actual_mbox,
		so_5::outliving_reference_t< trace_t > trace,
		std::string id )
	{
		return so_5::mbox_t{ so_5::stdcpp::make_unique< special_mbox_t >(
				std::move(actual_mbox),
				trace,
				std::move(id) )
		};
	}

	so_5::mbox_id_t
	id() const override { return m_actual_mbox->id(); }

	void
	subscribe_event_handler(
		const std::type_index & type_index,
		const so_5::message_limit::control_block_t * limit,
		so_5::agent_t * subscriber ) override
	{
		m_actual_mbox->subscribe_event_handler(
				type_index, limit, subscriber );
	}

	void
	unsubscribe_event_handlers(
		const std::type_index & type_index,
		so_5::agent_t * subscriber ) override
	{
		m_actual_mbox->unsubscribe_event_handlers( type_index, subscriber );
	}

	std::string
	query_name() const override
	{
		return m_actual_mbox->query_name();
	}

	so_5::mbox_type_t
	type() const override
	{
		return m_actual_mbox->type();
	}

	void
	do_deliver_message(
		const std::type_index & msg_type,
		const so_5::message_ref_t & message,
		unsigned int overlimit_reaction_deep ) const override
	{
		auto envelope = so_5::stdcpp::make_unique< Envelope >(
				m_trace,
				allocate_counter(),
				message );

		m_actual_mbox->do_deliver_enveloped_msg(
				msg_type,
				so_5::message_ref_t{ std::move(envelope) },
				overlimit_reaction_deep );
	}

	void
	do_deliver_service_request(
		const std::type_index & msg_type,
		const so_5::message_ref_t & message,
		unsigned int overlimit_reaction_deep ) const override
	{
		auto envelope = so_5::stdcpp::make_unique< Envelope >(
				m_trace,
				allocate_counter(),
				message );

		m_actual_mbox->do_deliver_enveloped_msg(
				msg_type,
				so_5::message_ref_t{ std::move(envelope) },
				overlimit_reaction_deep );
	}

	void
	do_deliver_enveloped_msg(
		const std::type_index & msg_type,
		const so_5::message_ref_t & message,
		unsigned int overlimit_reaction_deep ) override
	{
		auto envelope = so_5::stdcpp::make_unique< Envelope >(
				m_trace,
				allocate_counter(),
				message );

		m_actual_mbox->do_deliver_enveloped_msg(
				msg_type,
				so_5::message_ref_t{ std::move(envelope) },
				overlimit_reaction_deep );
	}

	void
	set_delivery_filter(
		const std::type_index & msg_type,
		const so_5::delivery_filter_t & filter,
		so_5::agent_t & subscriber ) override
	{
		m_actual_mbox->set_delivery_filter( msg_type, filter, subscriber );
	}

	void
	drop_delivery_filter(
		const std::type_index & msg_type,
		so_5::agent_t & subscriber ) SO_5_NOEXCEPT override
	{
		m_actual_mbox->drop_delivery_filter( msg_type, subscriber );
	}

protected :
	void
	do_deliver_message_from_timer(
		const std::type_index & msg_type,
		const so_5::message_ref_t & message ) override
	{
		auto envelope = so_5::stdcpp::make_unique< Envelope >(
				m_trace,
				allocate_counter(),
				message );

		delegate_deliver_message_from_timer(
				*m_actual_mbox,
				msg_type,
				so_5::message_ref_t{ std::move(envelope) } );
	}
};


/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Implementation of some helpers for message tracing.
 *
 * \since
 * v.5.5.22
 */

#include <so_5/rt/impl/h/msg_tracing_helpers.hpp>

namespace so_5 {

namespace impl {

namespace msg_tracing_helpers {

namespace details {

//
// actual_trace_data_t
//

optional<current_thread_id_t>
actual_trace_data_t::tid() const SO_5_NOEXCEPT
	{
		return m_tid;
	}

optional<const agent_t *>
actual_trace_data_t::agent() const SO_5_NOEXCEPT
	{
		return m_agent;
	}

optional<std::type_index>
actual_trace_data_t::msg_type() const SO_5_NOEXCEPT
	{
		return m_msg_type;
	}

optional<so_5::msg_tracing::msg_source_t>
actual_trace_data_t::msg_source() const SO_5_NOEXCEPT
	{
		return m_msg_source;
	}

optional<so_5::msg_tracing::message_or_signal_flag_t>
actual_trace_data_t::message_or_signal() const SO_5_NOEXCEPT
	{
		return m_message_or_signal;
	}

optional<so_5::msg_tracing::message_instance_info_t>
actual_trace_data_t::message_instance_info() const SO_5_NOEXCEPT
	{
		return m_message_instance_info;
	}

optional<so_5::msg_tracing::compound_action_description_t>
actual_trace_data_t::compound_action() const SO_5_NOEXCEPT
	{
		return m_compound_action;
	}

optional<const so_5::impl::event_handler_data_t *>
actual_trace_data_t::event_handler_data_ptr() const SO_5_NOEXCEPT
	{
		return m_event_handler_data_ptr;
	}

void
actual_trace_data_t::set_tid( current_thread_id_t tid ) SO_5_NOEXCEPT
	{
		m_tid = tid;
	}

void
actual_trace_data_t::set_agent( const agent_t * agent ) SO_5_NOEXCEPT
	{
		m_agent = agent;
	}

void
actual_trace_data_t::set_msg_type(
	const std::type_index & msg_type ) SO_5_NOEXCEPT
	{
		m_msg_type = msg_type;
	}

void
actual_trace_data_t::set_msg_source(
	so_5::msg_tracing::msg_source_t info ) SO_5_NOEXCEPT
	{
		m_msg_source = info;
	}

void
actual_trace_data_t::set_message_or_signal(
	so_5::msg_tracing::message_or_signal_flag_t flag ) SO_5_NOEXCEPT
	{
		m_message_or_signal = flag;
	}

void
actual_trace_data_t::set_message_instance_info(
	so_5::msg_tracing::message_instance_info_t info ) SO_5_NOEXCEPT
	{
		m_message_instance_info = info;
	}

void
actual_trace_data_t::set_compound_action(
	so_5::msg_tracing::compound_action_description_t desc ) SO_5_NOEXCEPT
	{
		m_compound_action = desc;
	}

void
actual_trace_data_t::set_event_handler_data_ptr(
	const so_5::impl::event_handler_data_t * ptr ) SO_5_NOEXCEPT
	{
		m_event_handler_data_ptr = ptr;
	}

} /* namespace details */

} /* namespace msg_tracing_helpers */

} /* namespace impl */

} /* namespace so_5 */


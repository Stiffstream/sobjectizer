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

#include <so_5/impl/msg_tracing_helpers.hpp>

namespace so_5 {

namespace impl {

namespace msg_tracing_helpers {

namespace details {

//
// actual_trace_data_t
//

optional<current_thread_id_t>
actual_trace_data_t::tid() const noexcept
	{
		return m_tid;
	}

optional<const message_sink_t *>
actual_trace_data_t::message_sink() const noexcept
	{
		return m_message_sink;
	}

optional<std::type_index>
actual_trace_data_t::msg_type() const noexcept
	{
		return m_msg_type;
	}

optional<so_5::msg_tracing::msg_source_t>
actual_trace_data_t::msg_source() const noexcept
	{
		return m_msg_source;
	}

optional<so_5::msg_tracing::message_or_signal_flag_t>
actual_trace_data_t::message_or_signal() const noexcept
	{
		return m_message_or_signal;
	}

optional<so_5::msg_tracing::message_instance_info_t>
actual_trace_data_t::message_instance_info() const noexcept
	{
		return m_message_instance_info;
	}

optional<so_5::msg_tracing::compound_action_description_t>
actual_trace_data_t::compound_action() const noexcept
	{
		return m_compound_action;
	}

optional<const so_5::impl::event_handler_data_t *>
actual_trace_data_t::event_handler_data_ptr() const noexcept
	{
		return m_event_handler_data_ptr;
	}

void
actual_trace_data_t::set_tid( current_thread_id_t tid ) noexcept
	{
		m_tid = tid;
	}

void
actual_trace_data_t::set_message_sink( const message_sink_t * sink ) noexcept
	{
		m_message_sink = sink;
	}

void
actual_trace_data_t::set_msg_type(
	const std::type_index & msg_type ) noexcept
	{
		m_msg_type = msg_type;
	}

void
actual_trace_data_t::set_msg_source(
	so_5::msg_tracing::msg_source_t info ) noexcept
	{
		m_msg_source = info;
	}

void
actual_trace_data_t::set_message_or_signal(
	so_5::msg_tracing::message_or_signal_flag_t flag ) noexcept
	{
		m_message_or_signal = flag;
	}

void
actual_trace_data_t::set_message_instance_info(
	so_5::msg_tracing::message_instance_info_t info ) noexcept
	{
		m_message_instance_info = info;
	}

void
actual_trace_data_t::set_compound_action(
	so_5::msg_tracing::compound_action_description_t desc ) noexcept
	{
		m_compound_action = desc;
	}

void
actual_trace_data_t::set_event_handler_data_ptr(
	const so_5::impl::event_handler_data_t * ptr ) noexcept
	{
		m_event_handler_data_ptr = ptr;
	}

} /* namespace details */

} /* namespace msg_tracing_helpers */

} /* namespace impl */

} /* namespace so_5 */


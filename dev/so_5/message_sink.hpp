/*
	SObjectizer 5.
*/

/*!
	\file
	\brief A base class for message sinks.
	\since v.5.8.0
*/

#pragma once

#include <so_5/message.hpp>
#include <so_5/priority.hpp>

namespace so_5
{

//
// message_sink_t
//
//FIXME: document this!
class SO_5_TYPE message_sink_t
	{
	public:
		virtual ~message_sink_t() noexcept = default;

		[[nodiscard]]
		virtual environment_t &
		so_environment() const noexcept = 0;

		[[nodiscard]]
		virtual priority_t
		so_message_sink_priority() const noexcept = 0;

		virtual void
		so_push_event(
			const message_limit::control_block_t * limit,
			mbox_id_t mbox_id,
			std::type_index msg_type,
			const message_ref_t & message ) = 0;
	};

} /* namespace so_5 */


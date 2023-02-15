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
		environment() const noexcept = 0;

		[[nodiscard]]
		virtual priority_t
		sink_priority() const noexcept = 0;

		virtual void
		push_event(
			mbox_id_t mbox_id,
			const std::type_index & msg_type,
			const message_ref_t & message,
			unsigned int overlimit_reaction_deep,
			//! Message delivery tracer to be used inside overlimit reaction.
			const message_limit::impl::action_msg_tracer_t * tracer ) = 0;
	};

} /* namespace so_5 */


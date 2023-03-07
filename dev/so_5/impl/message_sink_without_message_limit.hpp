/*
 * SObjectizer-5
 */

/*!
 * \since v.5.8.0
 *
 * \brief Implementation of message sink that ignores message limits.
 */

#pragma once

#include <so_5/impl/message_sink_for_agent.hpp>

namespace so_5
{

namespace impl
{

//
// message_sink_without_message_limit_t
//
/*!
 * \brief Implementation of message_sink that has no message limits.
 *
 * \since v.5.8.0
 */
class message_sink_without_message_limit_t final
	: public message_sink_for_agent_t
	{
	public:
		//! Constructor for a case when agent is being created.
		message_sink_without_message_limit_t(
			partially_constructed_agent_ptr_t owner_ptr )
			:	message_sink_for_agent_t{ owner_ptr }
			{}

		//! Constructor for a case when agent is already created.
		message_sink_without_message_limit_t(
			outliving_reference_t< agent_t > owner )
			:	message_sink_for_agent_t{ owner }
			{}

		void
		push_event(
			mbox_id_t mbox_id,
			message_delivery_mode_t /*delivery_mode*/,
			const std::type_index & msg_type,
			const message_ref_t & message,
			unsigned int /*redirection_deep*/,
			const message_limit::impl::action_msg_tracer_t * tracer )
			{
				// The fact of pushing message to the queue
				// has to be logged if msg_tracing is on.
				if( tracer )
					tracer->push_to_queue( this, owner_pointer() );

				agent_t::call_push_event(
						owner_reference(),
						nullptr /* no message limit */,
						mbox_id,
						msg_type,
						message );
			}
	};

} /* namespace impl */

} /* namespace so_5 */


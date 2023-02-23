/*
 * SObjectizer-5
 */

/*!
 * \since v.5.8.0
 *
 * \brief Implementation of message sink that ignores message limits.
 */

#pragma once

#include <so_5/message_limit.hpp>

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
	: public message_sink_t
	{
		//FIXME: should it be a reference instead of a pointer?
		//! Owner of the sink.
		agent_t * m_owner;

	public:
		//! Constructor for a case when agent is being created.
		message_sink_without_message_limit_t(
			partially_constructed_agent_ptr_t owner_ptr )
			:	m_owner( owner_ptr.ptr() )
			{}

		//! Constructor for a case when agent is already created.
		message_sink_without_message_limit_t(
			outliving_reference_t< agent_t > owner )
			:	m_owner( std::addressof(owner.get()) )
			{}

		environment_t &
		environment() const noexcept override
			{
				return m_owner->so_environment();
			}

		priority_t
		sink_priority() const noexcept override
			{
				return m_owner->so_priority();
			}

		void
		push_event(
			mbox_id_t mbox_id,
			const std::type_index & msg_type,
			const message_ref_t & message,
			unsigned int /*overlimit_reaction_deep*/,
			const message_limit::impl::action_msg_tracer_t * tracer )
			{
				// The fact of pushing message to the queue
				// has to be logged if msg_tracing is on.
				if( tracer )
					tracer->push_to_queue( this, m_owner );

				agent_t::call_push_event(
						*m_owner,
						nullptr /* no message limit */,
						mbox_id,
						msg_type,
						message );
			}

		[[nodiscard]]
		agent_t &
		owner() const noexcept
			{
				return *m_owner;
			}
	};

} /* namespace impl */

} /* namespace so_5 */


/*
 * SObjectizer-5
 */

/*!
 * \since v.5.8.0
 *
 * \brief Implementation of message sink that handles message limits.
 */

#pragma once

#include <so_5/impl/message_sink_for_agent.hpp>

namespace so_5
{

namespace impl
{

namespace push_event_impl
{

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunneeded-member-function"
#pragma clang diagnostic ignored "-Wunused-member-function"
#endif

/*!
 * \since
 * v.5.5.4
 *
 * \brief Helper class to rollback message count change in
 * case of an exception.
 */
struct decrement_on_exception_t
{
	const so_5::message_limit::control_block_t * m_limit;
	bool m_commited = false;

	decrement_on_exception_t(
		const so_5::message_limit::control_block_t * limit )
		:	m_limit( limit )
	{}
	~decrement_on_exception_t()
	{
		if( !m_commited )
			--(m_limit->m_count);
	}

	void
	commit() { m_commited = true; }
};

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

} /* namespace push_event_impl */

//
// message_sink_with_message_limit_t
//
/*!
 * \brief Implementation of message_sink that handles message limits.
 *
 * \since v.5.8.0
 */
class message_sink_with_message_limit_t final
	: public message_sink_for_agent_t
	{
		//! Run-time data for the message type.
		so_5::message_limit::control_block_t m_control_block;

	public:
		//! Initializing constructor.
		message_sink_with_message_limit_t(
			//! Owner of the sink.
			partially_constructed_agent_ptr_t owner_ptr,
			//! Limit for that message type.
			unsigned int limit,
			//! Reaction to the limit overflow.
			so_5::message_limit::action_t action )
			:	message_sink_for_agent_t( owner_ptr )
			,	m_control_block( limit, std::move( action ) )
			{}

		void
		push_event(
			mbox_id_t mbox_id,
			const std::type_index & msg_type,
			const message_ref_t & message,
			unsigned int overlimit_reaction_deep,
			const message_limit::impl::action_msg_tracer_t * tracer )
			{
				if( m_control_block.m_limit < ++(m_control_block.m_count) )
					{
						--(m_control_block.m_count);

						m_control_block.m_action(
								so_5::message_limit::overlimit_context_t{
										mbox_id,
										owner_reference(),
										m_control_block,
										overlimit_reaction_deep,
										msg_type,
										message,
										tracer
								} );
					}
				else
					{
						using namespace push_event_impl;

						decrement_on_exception_t exception_guard(
								std::addressof( m_control_block ) );

						// The fact of pushing message to the queue
						// has to be logged if msg_tracing is on.
						if( tracer )
							tracer->push_to_queue( this, owner_pointer() );

						agent_t::call_push_event(
								owner_reference(),
								std::addressof( m_control_block ),
								mbox_id,
								msg_type,
								message );

						exception_guard.commit();
					}
			}
	};

} /* namespace impl */

} /* namespace so_5 */


/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.5.9
 *
 * \file
 * \brief Declaration of action_msg_tracer for overlimit action tracing.
 */

#pragma once

#include <so_5/rt/h/message.hpp>

namespace so_5 {

namespace message_limit {

namespace impl {

//
// action_msg_tracer_t
//
/*!
 * \since
 * v.5.5.9
 *
 * \brief An interface of tracer for message delivery tracing.
 */
class action_msg_tracer_t
	{
	protected :
		~action_msg_tracer_t() {}

	public :
		//! Application will be aborted as result of overlimit.
		virtual void
		reaction_abort_app(
			//! Agent-receiver for the problematic message.
			const agent_t * subscriber ) const SO_5_NOEXCEPT = 0;

		//! Message will be dropped as result of overlimit.
		virtual void
		reaction_drop_message(
			//! Agent-receiver for the problematic message.
			const agent_t * subscriber ) const SO_5_NOEXCEPT = 0;

		//! Message will be redirected to another mbox.
		virtual void
		reaction_redirect_message(
			//! Agent-receiver for the problematic message.
			const agent_t * subscriber,
			//! A new destination for the message.
			const mbox_t & target ) const SO_5_NOEXCEPT = 0;

		//! Message will be transformed and redirected.
		virtual void
		reaction_transform(
			//! Agent-receiver for the problematic message.
			const agent_t * subscriber,
			//! A new destination for transformed message.
			const mbox_t & target,
			//! A new type of message.
			const std::type_index & msg_type,
			//! An instance of new message.
			const message_ref_t & transformed ) const SO_5_NOEXCEPT = 0;
	};

} /* namespace impl */

} /* namespace message_limit */

} /* namespace so_5 */


/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.5.4
 *
 * \brief Public part of message limit implementation.
 */

#include <so_5/message_limit.hpp>

#include <so_5/impl/message_limit_action_msg_tracer.hpp>
#include <so_5/impl/enveloped_msg_details.hpp>

#include <so_5/environment.hpp>
#include <so_5/enveloped_msg.hpp>

#include <so_5/error_logger.hpp>
#include <so_5/ret_code.hpp>


#include <so_5/details/abort_on_fatal_error.hpp>

#include <sstream>

namespace so_5
{

namespace message_limit
{

namespace impl
{

/*!
 * \since
 * v.5.5.4
 *
 * \brief Maximum overlimit reaction deep.
 */
const unsigned int max_overlimit_reaction_deep = 32;

SO_5_FUNC
void
drop_message_reaction( const overlimit_context_t & ctx )
	{
		if( ctx.m_msg_tracer )
			ctx.m_msg_tracer->reaction_drop_message( &ctx.m_receiver );
	}

SO_5_FUNC
void
abort_app_reaction( const overlimit_context_t & ctx )
	{
		so_5::details::abort_on_fatal_error( [&] {
			if( ctx.m_msg_tracer )
				ctx.m_msg_tracer->reaction_abort_app( &ctx.m_receiver );

			SO_5_LOG_ERROR( ctx.m_receiver.so_environment().error_logger(), logger )
				logger
					<< "message limit exceeded, application will be aborted. "
					<< " msg_type: " << ctx.m_msg_type.name()
					<< ", limit: " << ctx.m_limit.m_limit
					<< ", agent: " << &(ctx.m_receiver)
					<< std::endl;
		} );
	}

SO_5_FUNC
void
redirect_reaction(
	const overlimit_context_t & ctx,
	const mbox_t & to )
	{
		if( ctx.m_reaction_deep >= max_overlimit_reaction_deep )
			{
				// NOTE: this fragment can throw but it isn't a problem
				// because redirect_reaction() is called during message
				// delivery process and exceptions are expected in that
				// process.
				SO_5_LOG_ERROR(
						ctx.m_receiver.so_environment().error_logger(),
						logger )
					logger << "maximum message reaction deep exceeded on "
							"redirection; message will be ignored; "
						<< " msg_type: " << ctx.m_msg_type.name()
						<< ", limit: " << ctx.m_limit.m_limit
						<< ", agent: " << &(ctx.m_receiver)
						<< ", target_mbox: " << to->query_name();
			}
		else
			{
				if( ctx.m_msg_tracer )
					ctx.m_msg_tracer->reaction_redirect_message(
							&ctx.m_receiver,
							to );

				// Since v.5.8.0 nonblocking delivery mode
				// has to be used for redirection.
				// Otherwise the timer thread can be blocked if
				// the destination is a full mchain.
				to->do_deliver_message(
						message_delivery_mode_t::nonblocking,
						ctx.m_msg_type,
						ctx.m_message,
						ctx.m_reaction_deep + 1 );
			}
	}

SO_5_FUNC
void
transform_reaction(
	const overlimit_context_t & ctx,
	const mbox_t & to,
	const std::type_index & msg_type,
	const message_ref_t & message )
	{
		if( ctx.m_reaction_deep >= max_overlimit_reaction_deep )
			{
				// NOTE: this fragment can throw but it isn't a problem
				// because transform_reaction() is called during message
				// delivery process and exceptions are expected in that
				// process.
				SO_5_LOG_ERROR(
						ctx.m_receiver.so_environment().error_logger(),
						logger )
					logger << "maximum message reaction deep exceeded on "
							"transformation; message will be ignored;"
						<< " original_msg_type: " << ctx.m_msg_type.name()
						<< ", limit: " << ctx.m_limit.m_limit
						<< ", agent: " << &(ctx.m_receiver)
						<< ", result_msg_type: " << msg_type.name()
						<< ", target_mbox: " << to->query_name();
			}
		else
			{
				if( ctx.m_msg_tracer )
					ctx.m_msg_tracer->reaction_transform(
							&ctx.m_receiver,
							to,
							msg_type,
							message );

				// Since v.5.8.0 nonblocking delivery mode
				// has to be used for redirection.
				// Otherwise the timer thread can be blocked if
				// the destination is a full mchain.
				to->do_deliver_message(
						message_delivery_mode_t::nonblocking,
						msg_type,
						message,
						ctx.m_reaction_deep + 1 );
			}
	}

} /* namespace impl */

} /* namespace message_limit */

} /* namespace so_5 */


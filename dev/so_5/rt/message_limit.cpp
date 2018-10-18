/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.5.4
 *
 * \brief Public part of message limit implementation.
 */

#include <so_5/rt/h/message_limit.hpp>

#include <so_5/rt/impl/h/message_limit_action_msg_tracer.hpp>
#include <so_5/rt/impl/h/enveloped_msg_details.hpp>

#include <so_5/rt/h/environment.hpp>
#include <so_5/rt/h/enveloped_msg.hpp>

#include <so_5/h/error_logger.hpp>
#include <so_5/h/ret_code.hpp>


#include <so_5/details/h/abort_on_fatal_error.hpp>

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

				switch( ctx.m_event_type )
					{
					case invocation_type_t::event:
						to->do_deliver_message(
								ctx.m_msg_type,
								ctx.m_message,
								ctx.m_reaction_deep + 1 );
					break;

					case invocation_type_t::service_request:
						to->do_deliver_service_request(
								ctx.m_msg_type,
								ctx.m_message,
								ctx.m_reaction_deep + 1 );
					break;

					case invocation_type_t::enveloped_msg:
						to->do_deliver_enveloped_msg(
								ctx.m_msg_type,
								ctx.m_message,
								ctx.m_reaction_deep + 1 );
					break;
					}
			}
	}

namespace {

void
throw_exception_about_service_request_transformation(
	const overlimit_context_t & ctx )
	{
		std::ostringstream ss;
		ss << "service_request cannot be transformed;"
				<< " msg_type: " << ctx.m_msg_type.name()
				<< ", limit: " << ctx.m_limit.m_limit
				<< ", agent: " << &(ctx.m_receiver);
		SO_5_THROW_EXCEPTION(
				rc_svc_request_cannot_be_transfomred_on_overlimit,
				ss.str() );
	}

} /* namespace anonymous */

SO_5_FUNC
void
ensure_event_transform_reaction(
	invocation_type_t invocation_type,
	const overlimit_context_t & ctx )
{
	if( invocation_type_t::service_request == invocation_type )
		throw_exception_about_service_request_transformation( ctx );
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

				to->do_deliver_message(
						msg_type,
						message,
						ctx.m_reaction_deep + 1 );
			}
	}

} /* namespace impl */

} /* namespace message_limit */

} /* namespace so_5 */


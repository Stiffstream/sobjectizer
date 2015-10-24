/*
	SObjectizer 5.
*/

/*!
	\file
	\brief A local mbox definition.
*/

#if !defined( _SO_5__RT__IMPL__LOCAL_MBOX_HPP_ )
#define _SO_5__RT__IMPL__LOCAL_MBOX_HPP_

#include <map>
#include <vector>

#include <so_5/h/types.hpp>
#include <so_5/h/exception.hpp>

#include <so_5/h/spinlocks.hpp>

#include <so_5/h/msg_tracing.hpp>

#include <so_5/rt/h/mbox.hpp>
#include <so_5/rt/h/agent.hpp>

#include <so_5/rt/impl/h/agent_ptr_compare.hpp>
#include <so_5/rt/impl/h/message_limit_internals.hpp>
#include <so_5/rt/impl/h/msg_tracing_helpers.hpp>

namespace so_5
{

namespace rt
{

namespace impl
{

namespace local_mbox_details
{

//
// data_t
//

/*!
 * \since v.5.5.9
 * \brief A coolection of data required for local mbox implementation.
 */
struct data_t
	{
		data_t( mbox_id_t id )
			:	m_id{ id }
			{}

		//! ID of this mbox.
		const mbox_id_t m_id;

		//! Object lock.
		mutable default_rw_spinlock_t m_lock;

		/*!
		 * \since v.5.5.4
		 * \brief An information block about one subscriber.
		 */
		class subscriber_info_t
		{
			/*!
			 * \since v.5.5.5
			 * \brief Current status of the subscriber.
			 */
			enum class state_t
			{
				nothing,
				only_subscriptions,
				only_filter,
				subscriptions_and_filter
			};

			//! Subscriber.
			agent_t * m_agent;

			//! Optional message limit for that subscriber.
			const so_5::rt::message_limit::control_block_t * m_limit;

			/*!
			 * \since v.5.5.5
			 * \brief Delivery filter for that message for that subscriber.
			 */
			const delivery_filter_t * m_filter;

			/*!
			 * \since v.5.5.5
			 * \brief Current state of the subscriber parameters.
			 */
			state_t m_state;

		public :
			//! Constructor for the case when info object is created only
			//! for searching of existing subscription info.
			subscriber_info_t( agent_t * agent )
				:	m_agent( agent )
				,	m_limit( nullptr )
				,	m_filter( nullptr )
				,	m_state( state_t::nothing )
			{}

			//! Constructor for the case when subscriber info is being
			//! created during event subscription.
			subscriber_info_t(
				agent_t * agent,
				const so_5::rt::message_limit::control_block_t * limit )
				:	m_agent( agent )
				,	m_limit( limit )
				,	m_filter( nullptr )
				,	m_state( state_t::only_subscriptions )
			{}

			//! Constructor for the case when subscriber info is being
			//! created during event subscription.
			subscriber_info_t(
				agent_t * agent,
				const delivery_filter_t * filter )
				:	m_agent( agent )
				,	m_limit( nullptr )
				,	m_filter( filter )
				,	m_state( state_t::only_filter )
			{}

			//! Comparison uses only pointer to subscriber.
			/*!
			 * \note Since v.5.5.8 not only agent pointer, but also
			 * the priorities of the agents are used in comparision.
			 */
			bool
			operator<( const subscriber_info_t & o ) const
			{
				return special_agent_ptr_compare( *m_agent, *o.m_agent );
			}

			bool
			empty() const
			{
				return state_t::nothing == m_state;
			}

			agent_t &
			subscriber() const
			{
				return *m_agent;
			}

			const message_limit::control_block_t *
			limit() const
			{
				return m_limit;
			}

			//! Set the message limit for the subscriber.
			/*!
			 * Setting the message limit means that there are subscriptions
			 * for the agent.
			 *
			 * \note The message limit can be nullptr.
			 */
			void
			set_limit( const message_limit::control_block_t * limit )
			{
				m_limit = limit;

				m_state = ( state_t::nothing == m_state ?
						state_t::only_subscriptions :
						state_t::subscriptions_and_filter );
			}

			//! Drop the message limit for the subscriber.
			/*!
			 * Dropping the message limit means that there is no more
			 * subscription for the agent.
			 */
			void
			drop_limit()
			{
				m_limit = nullptr;

				m_state = ( state_t::only_subscriptions == m_state ?
						state_t::nothing : state_t::only_filter );
			}

			//! Set the delivery filter for the subscriber.
			void
			set_filter( const delivery_filter_t & filter )
			{
				m_filter = &filter;

				m_state = ( state_t::nothing == m_state ?
						state_t::only_filter :
						state_t::subscriptions_and_filter );
			}

			//! Drop the delivery filter for the subscriber.
			void
			drop_filter()
			{
				m_filter = nullptr;

				m_state = ( state_t::only_filter == m_state ?
						state_t::nothing : state_t::only_subscriptions );
			}

			//! Must a message be delivered to the subscriber?
			delivery_possibility_t
			must_be_delivered(
				const message_t & msg ) const
			{
				// For the case when there are actual subscriptions.
				// We assume that will be in 99.9% cases.
				auto need_deliver = delivery_possibility_t::must_be_delivered;

				if( state_t::only_filter == m_state )
					// Only filter, no actual subscriptions.
					// No message delivery for that case.
					need_deliver = delivery_possibility_t::no_subscription;
				else if( state_t::subscriptions_and_filter == m_state )
					// Delivery must be checked by delivery filter.
					need_deliver = m_filter->check( subscriber(), msg ) ?
							delivery_possibility_t::must_be_delivered :
							delivery_possibility_t::disabled_by_delivery_filter;

				return need_deliver;
			}
		};

		/*!
		 * \since v.5.4.0
		 * \brief Type of container with subscribers to one message type.
		 */
		typedef std::vector< subscriber_info_t > subscriber_container_t;

		/*!
		 * \since v.5.4.0
		 * \brief Map from message type to subscribers.
		 */
		typedef std::map< std::type_index, subscriber_container_t >
				messages_table_t;

		//! Map of subscribers to messages.
		messages_table_t m_subscribers;
	};

} /* namespace local_mbox_details */

//
// local_mbox_template_t
//

//! A template with implementation of local mbox.
/*!
 * \since v.5.5.9
 * \tparam TRACING_BASE base class with implementation of message
 * delivery tracing methods.
 */
template< typename TRACING_BASE >
class local_mbox_template_t
	:	public abstract_message_box_t
	,	private local_mbox_details::data_t
	,	private TRACING_BASE
	{
	public:
		template< typename... TRACING_ARGS >
		local_mbox_template_t(
			//! ID of this mbox.
			mbox_id_t id,
			//! Optional parameters for TRACING_BASE's constructor.
			TRACING_ARGS &&... args )
			:	local_mbox_details::data_t{ id }
			,	TRACING_BASE{ std::forward< TRACING_ARGS >(args)... }
			{}

		virtual mbox_id_t
		id() const override
			{
				return m_id;
			}

		virtual void
		subscribe_event_handler(
			const std::type_index & type_wrapper,
			const so_5::rt::message_limit::control_block_t * limit,
			agent_t * subscriber ) override
			{
				std::unique_lock< default_rw_spinlock_t > lock( m_lock );

				auto it = m_subscribers.find( type_wrapper );
				if( it == m_subscribers.end() )
				{
					// There isn't such message type yet.
					subscriber_container_t container;
					container.emplace_back( subscriber, limit );

					m_subscribers.emplace( type_wrapper, std::move( container ) );
				}
				else
				{
					auto & agents = it->second;

					subscriber_info_t info{ subscriber, limit };

					auto pos = std::lower_bound( agents.begin(), agents.end(), info );
					if( pos != agents.end() )
					{
						// This is subscriber or appopriate place for it.
						if( &(pos->subscriber()) != subscriber )
							agents.insert( pos, info );
						else
							// Agent is already in subscribers list.
							// But its state must be updated.
							pos->set_limit( limit );
					}
					else
						agents.push_back( info );
				}
			}

		virtual void
		unsubscribe_event_handlers(
			const std::type_index & type_wrapper,
			agent_t * subscriber ) override
			{
				std::unique_lock< default_rw_spinlock_t > lock( m_lock );

				auto it = m_subscribers.find( type_wrapper );
				if( it != m_subscribers.end() )
				{
					auto & agents = it->second;

					auto pos = std::lower_bound( agents.begin(), agents.end(),
							subscriber_info_t{ subscriber } );
					if( pos != agents.end() && &(pos->subscriber()) == subscriber )
					{
						// Subscriber can be removed only if there is no delivery filter.
						pos->drop_limit();
						if( pos->empty() )
							agents.erase( pos );
					}

					if( agents.empty() )
						m_subscribers.erase( it );
				}
			}

		virtual std::string
		query_name() const override
			{
				std::ostringstream s;
				s << "<mbox:type=MPMC:id=" << m_id << ">";

				return s.str();
			}

		virtual mbox_type_t
		type() const override
			{
				return mbox_type_t::multi_producer_multi_consumer;
			}

		virtual void
		do_deliver_message(
			const std::type_index & msg_type,
			const message_ref_t & message,
			unsigned int overlimit_reaction_deep ) const override
			{
				typename TRACING_BASE::deliver_op_tracer_t tracer{
						*this, // as TRACING_BASE
						*this, // as abstract_message_box_t
						"deliver_message",
						msg_type, message, overlimit_reaction_deep };

				do_deliver_message_impl(
						tracer,
						msg_type,
						message,
						overlimit_reaction_deep );
			}

		virtual void
		do_deliver_service_request(
			const std::type_index & msg_type,
			const message_ref_t & message,
			unsigned int overlimit_reaction_deep ) const override
			{
				typename TRACING_BASE::deliver_op_tracer_t tracer{
						*this, // as TRACING_BASE
						*this, // as abstract_message_box_t
						"deliver_service_request",
						msg_type, message, overlimit_reaction_deep };

				do_deliver_service_request_impl(
						tracer,
						msg_type,
						message,
						overlimit_reaction_deep );
			}

		virtual void
		set_delivery_filter(
			const std::type_index & msg_type,
			const delivery_filter_t & filter,
			agent_t & subscriber ) override
			{
				std::unique_lock< default_rw_spinlock_t > lock( m_lock );

				auto it = m_subscribers.find( msg_type );
				if( it == m_subscribers.end() )
				{
					// There isn't such message type yet.
					subscriber_container_t container;
					container.emplace_back( &subscriber, &filter );

					m_subscribers.emplace( msg_type, std::move( container ) );
				}
				else
				{
					auto & agents = it->second;

					subscriber_info_t info{ &subscriber, &filter };

					auto pos = std::lower_bound( agents.begin(), agents.end(), info );
					if( pos != agents.end() )
					{
						// This is subscriber or appopriate place for it.
						if( &(pos->subscriber()) != &subscriber )
							agents.insert( pos, info );
						else
							// Agent is already in subscribers list.
							// But its state must be updated.
							pos->set_filter( filter );
					}
					else
						agents.push_back( info );
				}
			}

		virtual void
		drop_delivery_filter(
			const std::type_index & msg_type,
			agent_t & subscriber ) SO_5_NOEXCEPT override
			{
				std::unique_lock< default_rw_spinlock_t > lock( m_lock );

				auto it = m_subscribers.find( msg_type );
				if( it != m_subscribers.end() )
				{
					auto & agents = it->second;

					auto pos = std::lower_bound( agents.begin(), agents.end(),
							subscriber_info_t{ &subscriber } );
					if( pos != agents.end() && &(pos->subscriber()) == &subscriber )
					{
						// Subscriber can be removed only if there is no delivery filter.
						pos->drop_filter();
						if( pos->empty() )
							// There is no more need in that subscriber.
							agents.erase( pos );
					}

					if( agents.empty() )
						m_subscribers.erase( it );
				}
			}

	private :
		void
		do_deliver_message_impl(
			typename TRACING_BASE::deliver_op_tracer_t const & tracer,
			const std::type_index & msg_type,
			const message_ref_t & message,
			unsigned int overlimit_reaction_deep ) const
			{
				read_lock_guard_t< default_rw_spinlock_t > lock( m_lock );

				auto it = m_subscribers.find( msg_type );
				if( it != m_subscribers.end() )
					{
						for( const auto & a : it->second )
							do_deliver_message_to_subscriber(
									a,
									tracer,
									msg_type,
									message,
									overlimit_reaction_deep );
					}
				else
					tracer.no_subscribers();
			}

		void
		do_deliver_message_to_subscriber(
			const subscriber_info_t & agent_info,
			typename TRACING_BASE::deliver_op_tracer_t const & tracer,
			const std::type_index & msg_type,
			const message_ref_t & message,
			unsigned int overlimit_reaction_deep ) const
			{
				const auto delivery_status =
						agent_info.must_be_delivered( *(message.get()) );

				if( delivery_possibility_t::must_be_delivered == delivery_status )
					{
						using namespace so_5::rt::message_limit::impl;

						try_to_deliver_to_agent(
								invocation_type_t::event,
								agent_info.subscriber(),
								agent_info.limit(),
								msg_type,
								message,
								overlimit_reaction_deep,
								tracer.overlimit_tracer(),
								[&] {
									tracer.push_to_queue( &agent_info.subscriber() );

									agent_t::call_push_event(
											agent_info.subscriber(),
											agent_info.limit(),
											m_id,
											msg_type,
											message );
								} );
					}
				else
					tracer.message_rejected( &agent_info.subscriber(), delivery_status );
			}

		void
		do_deliver_service_request_impl(
			typename TRACING_BASE::deliver_op_tracer_t const & tracer,
			const std::type_index & msg_type,
			const message_ref_t & message,
			unsigned int overlimit_reaction_deep ) const
			{
				using namespace so_5::rt::message_limit::impl;

				msg_service_request_base_t::dispatch_wrapper( message,
					[&] {
						read_lock_guard_t< default_rw_spinlock_t > lock( m_lock );

						auto it = m_subscribers.find( msg_type );

						if( it == m_subscribers.end() )
							{
								tracer.no_subscribers();

								SO_5_THROW_EXCEPTION(
										so_5::rc_no_svc_handlers,
										"no service handlers (no subscribers for message)" );
							}

						if( 1 != it->second.size() )
							SO_5_THROW_EXCEPTION(
									so_5::rc_more_than_one_svc_handler,
									"more than one service handler found" );

						do_deliver_service_request_to_subscriber(
								tracer,
								it->second.front(),
								msg_type,
								message,
								overlimit_reaction_deep );
					} );
			}

		void
		do_deliver_service_request_to_subscriber(
			typename TRACING_BASE::deliver_op_tracer_t const & tracer,
			const subscriber_info_t & agent_info,
			const std::type_index & msg_type,
			const message_ref_t & message,
			unsigned int overlimit_reaction_deep ) const
			{
				const auto & svc_request_param =
					dynamic_cast< msg_service_request_base_t & >( *message )
							.query_param();

				const auto delivery_status =
						agent_info.must_be_delivered( svc_request_param );

				if( delivery_possibility_t::must_be_delivered == delivery_status )
					{
						using namespace so_5::rt::message_limit::impl;

						try_to_deliver_to_agent(
								invocation_type_t::service_request,
								agent_info.subscriber(),
								agent_info.limit(),
								msg_type,
								message,
								overlimit_reaction_deep,
								tracer.overlimit_tracer(),
								[&] {
									tracer.push_to_queue( &agent_info.subscriber() );

									agent_t::call_push_service_request(
											agent_info.subscriber(),
											agent_info.limit(),
											m_id,
											msg_type,
											message );
								} );
					}
				else
					{
						tracer.message_rejected(
								&agent_info.subscriber(),
								delivery_status );

						SO_5_THROW_EXCEPTION(
								so_5::rc_no_svc_handlers,
								"no service handlers (no subscribers for message or "
								"subscriber is blocked by delivery filter)" );
					}
			}
	};

/*!
 * \since v.5.5.9
 * \brief Alias for local mbox without message delivery tracing.
 */
using local_mbox_without_tracing_t =
	local_mbox_template_t< msg_tracing_helpers::tracing_disabled_base_t >;

/*!
 * \since v.5.5.9
 * \brief Alias for local mbox with message delivery tracing.
 */
using local_mbox_with_tracing_t =
	local_mbox_template_t< msg_tracing_helpers::tracing_enabled_base_t >;

} /* namespace impl */

} /* namespace rt */

} /* namespace so_5 */

#endif


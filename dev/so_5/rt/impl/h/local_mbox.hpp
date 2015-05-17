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

#include <so_5/rt/h/mbox.hpp>

namespace so_5
{

namespace rt
{

namespace impl
{

//
// local_mbox_t
//

//! A class for representing a local anonymous mbox.
class local_mbox_t : public abstract_message_box_t
{
	public:
		local_mbox_t( mbox_id_t id );
		virtual ~local_mbox_t();

		virtual mbox_id_t
		id() const override
			{
				return m_id;
			}

		virtual void
		subscribe_event_handler(
			const std::type_index & type_wrapper,
			const so_5::rt::message_limit::control_block_t * limit,
			agent_t * subscriber ) override;

		virtual void
		unsubscribe_event_handlers(
			const std::type_index & type_wrapper,
			agent_t * subscriber ) override;

		virtual std::string
		query_name() const override;

		virtual mbox_type_t
		type() const override
			{
				return mbox_type_t::multi_producer_multi_consumer;
			}

		virtual void
		do_deliver_message(
			const std::type_index & msg_type,
			const message_ref_t & message,
			unsigned int overlimit_reaction_deep ) const override;

		virtual void
		do_deliver_service_request(
			const std::type_index & msg_type,
			const message_ref_t & message,
			unsigned int overlimit_reaction_deep ) const override;

		virtual void
		set_delivery_filter(
			const std::type_index & msg_type,
			const delivery_filter_t & filter,
			agent_t & subscriber ) override;

		virtual void
		drop_delivery_filter(
			const std::type_index & msg_type,
			agent_t & subscriber ) SO_5_NOEXCEPT override;

	private:
		/*!
		 * \since v.5.4.0
		 * \brief ID of this mbox.
		 */
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
			bool
			operator<( const subscriber_info_t & o ) const
			{
				return m_agent < o.m_agent;
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
			bool
			must_be_delivered(
				const message_t & msg ) const
			{
				// For the case when there are actual subscriptions.
				// We assume that will be in 99.9% cases.
				bool need_deliver = true;

				if( state_t::only_filter == m_state )
					// Only filter, no actual subscriptions.
					// No message delivery for that case.
					need_deliver = false;
				else if( state_t::subscriptions_and_filter == m_state )
					// Delivery must be checked by delivery filter.
					need_deliver = m_filter->check( subscriber(), msg );

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

} /* namespace impl */

} /* namespace rt */

} /* namespace so_5 */

#endif

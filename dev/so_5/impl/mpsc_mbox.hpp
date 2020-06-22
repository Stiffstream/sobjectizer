/*
	SObjectizer 5.
*/

/*!
	\file
	\since
	v.5.4.0

	\brief A multi-producer/single-consumer mbox definition.
*/

#pragma once

#include <so_5/types.hpp>
#include <so_5/exception.hpp>
#include <so_5/spinlocks.hpp>

#include <so_5/mbox.hpp>
#include <so_5/event_queue.hpp>
#include <so_5/message_limit.hpp>

#include <so_5/impl/msg_tracing_helpers.hpp>
#include <so_5/impl/message_limit_internals.hpp>

namespace so_5
{

namespace impl
{

//
// limitless_mpsc_mbox_template_t
//

/*!
 * \since
 * v.5.4.0, v.5.7.1
 *
 * \brief A multi-producer/single-consumer mbox definition.
 *
 * Should be used in cases where limits are not used.
 *
 * \note Renamed from limitless_mpsc_mbox_t to limitless_mpsc_mbox_template
 * in v.5.5.9.
 * \note Renamed from limitless_mpsc_mbox_template to
 * limitless_mpsc_mbox_template_t in v.5.7.1.
 */
template< typename Tracing_Base >
class limitless_mpsc_mbox_template_t
	:	public abstract_message_box_t
	,	protected Tracing_Base
	{
	public:
		template< typename... Tracing_Args >
		limitless_mpsc_mbox_template_t(
			mbox_id_t id,
			agent_t * single_consumer,
			Tracing_Args &&... tracing_args )
			:	Tracing_Base{ std::forward< Tracing_Args >( tracing_args )... }
			,	m_id{ id }
			,	m_single_consumer{ single_consumer }
			{}

		mbox_id_t
		id() const override
			{
				return m_id;
			}

		void
		subscribe_event_handler(
			const std::type_index & msg_type,
			const message_limit::control_block_t * /*limit*/,
			agent_t & subscriber ) override
			{
				std::lock_guard< default_rw_spinlock_t > lock{ m_lock };

				if( &subscriber != m_single_consumer )
					SO_5_THROW_EXCEPTION(
							rc_illegal_subscriber_for_mpsc_mbox,
							"the only one consumer can create subscription to mpsc_mbox" );

				m_subscriptions.emplace( msg_type );
			}

		void
		unsubscribe_event_handlers(
			const std::type_index & msg_type,
			agent_t & subscriber ) override
			{
				std::lock_guard< default_rw_spinlock_t > lock{ m_lock };

				if( &subscriber != m_single_consumer )
					SO_5_THROW_EXCEPTION(
							rc_illegal_subscriber_for_mpsc_mbox,
							"the only one consumer can remove subscription to mpsc_mbox" );

				m_subscriptions.erase( msg_type );
			}

		std::string
		query_name() const override
			{
				std::ostringstream s;
				s << "<mbox:type=limitlesssMPSC:id="
						<< m_id << ":consumer=" << m_single_consumer
						<< ">";

				return s.str();
			}

		mbox_type_t
		type() const override
			{
				return mbox_type_t::multi_producer_single_consumer;
			}

		void
		do_deliver_message(
			const std::type_index & msg_type,
			const message_ref_t & message,
			unsigned int overlimit_reaction_deep ) override
			{
				typename Tracing_Base::deliver_op_tracer tracer{
						*this, // as Tracing_Base
						*this, // as abstract_message_box_t
						"deliver_message",
						msg_type, message, overlimit_reaction_deep };

				this->do_delivery( msg_type, tracer,
					[&]()
					{
						using namespace so_5::message_limit::impl;

						try_to_deliver_to_agent(
								this->m_id,
								*(this->m_single_consumer),
								nullptr, // No limit.
								msg_type,
								message,
								overlimit_reaction_deep,
								tracer.overlimit_tracer(),
								[&] {
									tracer.push_to_queue( this->m_single_consumer );

									agent_t::call_push_event(
											*(this->m_single_consumer),
											nullptr, // No limit.
											this->m_id,
											msg_type,
											message );
								} );
					} );
			}

		/*!
		 * \attention Will throw an exception because delivery
		 * filter is not applicable to MPSC-mboxes.
		 */
		void
		set_delivery_filter(
			const std::type_index & /*msg_type*/,
			const delivery_filter_t & /*filter*/,
			agent_t & /*subscriber*/ ) override
			{
				SO_5_THROW_EXCEPTION(
						rc_delivery_filter_cannot_be_used_on_mpsc_mbox,
						"set_delivery_filter is called for MPSC-mbox" );
			}

		void
		drop_delivery_filter(
			const std::type_index & /*msg_type*/,
			agent_t & /*subscriber*/ ) noexcept override
			{}

		environment_t &
		environment() const noexcept override
			{
				return m_single_consumer->so_environment();
			}

	protected :
		/*!
		 * \brief Type of dictionary for information about the current
		 * subscriptions.
		 *
		 * \since
		 * v.5.7.1
		 */
		using subscriptions_set_t = std::set< std::type_index >;

		/*!
		 * \brief ID of this mbox.
		 */
		const mbox_id_t m_id;

		//! The only consumer of this mbox's messages.
		agent_t * m_single_consumer;

		/*!
		 * \since
		 * v.5.5.9
		 *
		 * \brief Protection of object from modification.
		 */
		default_rw_spinlock_t m_lock;

		/*!
		 * \brief Information about the current subscriptions.
		 *
		 * \since
		 * v.5.7.1
		 */
		subscriptions_set_t m_subscriptions;

		/*!
		 * \since
		 * v.5.5.9
		 *
		 * \brief Helper method to do delivery actions under locked object.
		 *
		 * \tparam L lambda with actual delivery actions.
		 */
		template< typename L >
		void
		do_delivery(
			//! Type of message/signal to be delivered.
			const std::type_index & msg_type,
			//! Tracer object to log the case of abscense of subscriptions.
			typename Tracing_Base::deliver_op_tracer const & tracer,
			//! Lambda with actual delivery actions.
			L l )
		{
			read_lock_guard_t< default_rw_spinlock_t > lock{ m_lock };

			const auto it = m_subscriptions.find( msg_type );
			if( it != m_subscriptions.end() )
				l();
			else
				tracer.no_subscribers();
		}
	};

//
// limitful_mpsc_mbox_template_t
//

/*!
 * \since
 * v.5.4.0, v.5.7.1
 *
 * \brief A multi-producer/single-consumer mbox definition.
 *
 * \note Renamed from limitful_mpsc_mbox_t to limitful_mpsc_mbox_template
 * in v.5.5.9.
 * \note Renamed from limitful_mpsc_mbox_template to
 * limitful_mpsc_mbox_template_t in v.5.7.1.
 */
template< typename Tracing_Base >
class limitful_mpsc_mbox_template_t
	:	public abstract_message_box_t
	,	protected Tracing_Base
	{
	public:
		template< typename... Tracing_Args >
		limitful_mpsc_mbox_template_t(
			mbox_id_t id,
			agent_t * single_consumer,
			Tracing_Args &&... tracing_args )
			:	Tracing_Base{ std::forward< Tracing_Args >( tracing_args )... }
			,	m_id{ id }
			,	m_single_consumer{ single_consumer }
			{}

		mbox_id_t
		id() const override
			{
				return m_id;
			}

		void
		subscribe_event_handler(
			const std::type_index & msg_type,
			const message_limit::control_block_t * limit,
			agent_t & subscriber ) override
			{
				std::lock_guard< default_rw_spinlock_t > lock{ m_lock };

				if( &subscriber != m_single_consumer )
					SO_5_THROW_EXCEPTION(
							rc_illegal_subscriber_for_mpsc_mbox,
							"the only one consumer can create subscription to mpsc_mbox" );

				m_subscriptions.emplace(
						msg_type,
						subscription_info_t{ limit } );
			}

		void
		unsubscribe_event_handlers(
			const std::type_index & msg_type,
			agent_t & subscriber ) override
			{
				std::lock_guard< default_rw_spinlock_t > lock{ m_lock };

				if( &subscriber != m_single_consumer )
					SO_5_THROW_EXCEPTION(
							rc_illegal_subscriber_for_mpsc_mbox,
							"the only one consumer can remove subscription to mpsc_mbox" );

				m_subscriptions.erase( msg_type );
			}

		std::string
		query_name() const override
			{
				std::ostringstream s;
				s << "<mbox:type=MPSC:id="
						<< m_id << ":consumer=" << m_single_consumer
						<< ">";

				return s.str();
			}

		mbox_type_t
		type() const override
			{
				return mbox_type_t::multi_producer_single_consumer;
			}

		void
		do_deliver_message(
			const std::type_index & msg_type,
			const message_ref_t & message,
			unsigned int overlimit_reaction_deep ) override
			{
				typename Tracing_Base::deliver_op_tracer tracer{
						*this, // as Tracing_Base
						*this, // as abstract_message_box_t
						"deliver_message",
						msg_type, message, overlimit_reaction_deep };

				this->do_delivery( msg_type, tracer,
					[&]( const subscription_info_t & info )
					{
						using namespace so_5::message_limit::impl;

						try_to_deliver_to_agent(
								this->m_id,
								*(this->m_single_consumer),
								info.m_limit,
								msg_type,
								message,
								overlimit_reaction_deep,
								tracer.overlimit_tracer(),
								[&] {
									tracer.push_to_queue( this->m_single_consumer );

									agent_t::call_push_event(
											*(this->m_single_consumer),
											info.m_limit,
											this->m_id,
											msg_type,
											message );
								} );
					} );
			}

		/*!
		 * \attention Will throw an exception because delivery
		 * filter is not applicable to MPSC-mboxes.
		 */
		void
		set_delivery_filter(
			const std::type_index & /*msg_type*/,
			const delivery_filter_t & /*filter*/,
			agent_t & /*subscriber*/ ) override
			{
				SO_5_THROW_EXCEPTION(
						rc_delivery_filter_cannot_be_used_on_mpsc_mbox,
						"set_delivery_filter is called for MPSC-mbox" );
			}

		void
		drop_delivery_filter(
			const std::type_index & /*msg_type*/,
			agent_t & /*subscriber*/ ) noexcept override
			{}

		environment_t &
		environment() const noexcept override
			{
				return m_single_consumer->so_environment();
			}

	protected :
		/*!
		 * \brief Information related to a subscribed message type.
		 *
		 * \since
		 * v.5.7.1
		 */
		struct subscription_info_t
			{
				//! Message limit for that type of messages.
				/*!
				 * Can be nullptr, if message limits aren't used.
				 */
				const message_limit::control_block_t * m_limit;
			};

		/*!
		 * \brief Type of dictionary for information about the current
		 * subscriptions.
		 *
		 * \since
		 * v.5.7.1
		 */
		using subscriptions_map_t = std::map<
				std::type_index,
				subscription_info_t >;

		/*!
		 * \brief ID of this mbox.
		 */
		const mbox_id_t m_id;

		//! The only consumer of this mbox's messages.
		agent_t * m_single_consumer;

		/*!
		 * \since
		 * v.5.5.9
		 *
		 * \brief Protection of object from modification.
		 */
		default_rw_spinlock_t m_lock;

		/*!
		 * \brief Information about the current subscriptions.
		 *
		 * \since
		 * v.5.7.1
		 */
		subscriptions_map_t m_subscriptions;

		/*!
		 * \since
		 * v.5.5.9
		 *
		 * \brief Helper method to do delivery actions under locked object.
		 *
		 * \tparam L lambda with actual delivery actions.
		 */
		template< typename L >
		void
		do_delivery(
			//! Type of message/signal to be delivered.
			const std::type_index & msg_type,
			//! Tracer object to log the case of abscense of subscriptions.
			typename Tracing_Base::deliver_op_tracer const & tracer,
			//! Lambda with actual delivery actions.
			L l )
		{
			read_lock_guard_t< default_rw_spinlock_t > lock{ m_lock };

			const auto it = m_subscriptions.find( msg_type );
			if( it != m_subscriptions.end() )
				l( it->second );
			else
				tracer.no_subscribers();
		}
	};

/*!
 * \since
 * v.5.5.9
 *
 * \brief Alias for limitless_mpsc_mbox without message delivery tracing.
 */
using limitless_mpsc_mbox_without_tracing_t =
	limitless_mpsc_mbox_template_t< msg_tracing_helpers::tracing_disabled_base >;

/*!
 * \since
 * v.5.5.9
 *
 * \brief Alias for limitless_mpsc_mbox with message delivery tracing.
 */
using limitless_mpsc_mbox_with_tracing_t =
	limitless_mpsc_mbox_template_t< msg_tracing_helpers::tracing_enabled_base >;

/*!
 * \since
 * v.5.5.9
 *
 * \brief Alias for limitful_mpsc_mbox without message delivery tracing.
 */
using limitful_mpsc_mbox_without_tracing_t =
	limitful_mpsc_mbox_template_t< msg_tracing_helpers::tracing_disabled_base >;

/*!
 * \since
 * v.5.5.9
 *
 * \brief Alias for limitful_mpsc_mbox with message delivery tracing.
 */
using limitful_mpsc_mbox_with_tracing_t =
	limitful_mpsc_mbox_template_t< msg_tracing_helpers::tracing_enabled_base >;

} /* namespace impl */

} /* namespace so_5 */


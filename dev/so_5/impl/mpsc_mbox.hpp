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

#include <so_5/impl/local_mbox_basic_subscription_info.hpp>

#include <so_5/impl/msg_tracing_helpers.hpp>
#include <so_5/impl/message_limit_internals.hpp>

#include <type_traits>

namespace so_5
{

namespace impl
{

//
// mpsc_mbox_with_message_limits_t
//
/*!
 * \brief Helper class to be used in limitful-MPSC mbox.
 *
 * Method message_limits_pointer() just returns an actual value of the argument.
 * It makes MPSC mbox limitful.
 *
 * \since v.5.7.4
 */
struct mpsc_mbox_with_message_limits_t
	{
		[[nodiscard]]
		static const so_5::message_limit::control_block_t *
		message_limits_pointer(
			const so_5::message_limit::control_block_t * limits ) noexcept
			{
				return limits;
			}
	};

//
// mpsc_mbox_without_message_limits_t
//
/*!
 * \brief Helper class to be used in limitless-MPSC mbox.
 *
 * Method message_limits_pointer() always returns nullptr.
 * It makes MPSC mbox limitless.
 *
 * \since v.5.7.4
 */
struct mpsc_mbox_without_message_limits_t
	{
		[[nodiscard]]
		static const so_5::message_limit::control_block_t *
		message_limits_pointer(
			const so_5::message_limit::control_block_t * /*limits*/ ) noexcept
			{
				return nullptr;
			}
	};

//
// mpsc_mbox_message_limits_usage_t
//
/*!
 * \brief Indicator for using message limits by MPSC mbox.
 *
 * \since v.5.7.4
 */
enum class mpsc_mbox_message_limits_usage_t
	{
		use,
		dont_use
	};

//
// mpsc_mbox_template_t
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
 * \note Renamed from limitful_mpsc_mbox_template_t to
 * mpsc_mbox_template_t in v.5.7.4.
 */
template<
	typename Tracing_Base,
	mpsc_mbox_message_limits_usage_t message_limits_usage >
class mpsc_mbox_template_t
	:	public abstract_message_box_t
	,	protected Tracing_Base
	{
		using limits_selector_t = std::conditional_t<
				mpsc_mbox_message_limits_usage_t::use == message_limits_usage,
				mpsc_mbox_with_message_limits_t,
				mpsc_mbox_without_message_limits_t >;

	public:
		template< typename... Tracing_Args >
		mpsc_mbox_template_t(
			mbox_id_t id,
			environment_t & env,
			agent_t * single_consumer,
			Tracing_Args &&... tracing_args )
			:	Tracing_Base{ std::forward< Tracing_Args >( tracing_args )... }
			,	m_id{ id }
			,	m_env{ env }
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

				insert_or_modify_subscription(
						msg_type,
						[&] {
							return subscription_info_t{
									limits_selector_t::message_limits_pointer( limit )
								};
						},
						[&]( subscription_info_t & info ) {
							info.set_limit(
									limits_selector_t::message_limits_pointer( limit ) );
						} );
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

				modify_and_remove_subscription_if_needed(
						msg_type,
						[]( subscription_info_t & info ) {
							info.drop_limit();
						} );
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

				this->do_delivery(
					msg_type,
					message,
					tracer,
					[&]( const subscription_info_t & info )
					{
						using namespace so_5::message_limit::impl;

						try_to_deliver_to_agent(
								this->m_id,
								*(this->m_single_consumer),
								info.limit(),
								msg_type,
								message,
								overlimit_reaction_deep,
								tracer.overlimit_tracer(),
								[&] {
									tracer.push_to_queue( this->m_single_consumer );

									agent_t::call_push_event(
											*(this->m_single_consumer),
											info.limit(),
											this->m_id,
											msg_type,
											message );
								} );
					} );
			}

		void
		set_delivery_filter(
			const std::type_index & msg_type,
			const delivery_filter_t & filter,
			agent_t & subscriber ) override
			{
				std::lock_guard< default_rw_spinlock_t > lock{ m_lock };

				if( &subscriber != m_single_consumer )
					SO_5_THROW_EXCEPTION(
							rc_illegal_subscriber_for_mpsc_mbox,
							"the only one consumer can create subscription to mpsc_mbox" );

				insert_or_modify_subscription(
						msg_type,
						[&] {
							return subscription_info_t{ &filter };
						},
						[&]( subscription_info_t & info ) {
							info.set_filter( filter );
						} );
			}

		void
		drop_delivery_filter(
			const std::type_index & msg_type,
			agent_t & subscriber ) noexcept override
			{
				std::lock_guard< default_rw_spinlock_t > lock{ m_lock };

				if( &subscriber != m_single_consumer )
					// Nothing to do (we can't throw an exception here).
					return;

				modify_and_remove_subscription_if_needed(
						msg_type,
						[]( subscription_info_t & info ) {
							info.drop_filter();
						} );
			}

		environment_t &
		environment() const noexcept override
			{
				return m_env;
			}

	protected :
		/*!
		 * \brief Information related to a subscribed message type.
		 *
		 * \since v.5.7.4
		 */
		using subscription_info_t = local_mbox_details::basic_subscription_info_t;

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

		/*!
		 * \brief Environment in that the mbox was created.
		 *
		 * \note
		 * Previous versions of SObjectizer didn't hold this reference
		 * in MPSC-mbox implementations. Instead, the reference was
		 * obtained via m_single_consumer. But this approach lead to
		 * access violation when MPSC-mbox's environment() method was
		 * called after the deregistration of m_single_consumer.
		 * Because of than the reference to SOEnv is stored inside
		 * MPSC-mbox and is returned by environment() method.
		 *
		 * \since v.5.7.5
		 */
		environment_t & m_env;

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
		 * \brief Helper for performing insertion or modification
		 * of subscription info.
		 *
		 * \since v.5.7.4
		 */
		template< typename Info_Maker, typename Info_Changer >
		void
		insert_or_modify_subscription(
			const std::type_index & msg_type,
			Info_Maker maker,
			Info_Changer changer )
			{
				auto it = m_subscriptions.find( msg_type );
				if( it == m_subscriptions.end() )
				{
					// Subscription for that type has to be created.
					m_subscriptions.emplace( msg_type, maker() );
				}
				else
				{
					// Subscription for that type has to be modified.
					changer( it->second );
				}
			}

		/*!
		 * \brief Helper for modification and deletion of
		 * subscription info.
		 *
		 * \since v.5.7.4
		 */
		template< typename Info_Changer >
		void
		modify_and_remove_subscription_if_needed(
			const std::type_index & msg_type,
			Info_Changer changer )
			{
				auto it = m_subscriptions.find( msg_type );
				if( it != m_subscriptions.end() )
				{
					changer( it->second );
					if( it->second.empty() )
						m_subscriptions.erase( it );
				}
			}

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
			//! Message object to be delivered.
			const message_ref_t & message,
			//! Tracer object to log the case of abscense of subscriptions.
			typename Tracing_Base::deliver_op_tracer const & tracer,
			//! Lambda with actual delivery actions.
			L l )
			{
				read_lock_guard_t< default_rw_spinlock_t > lock{ m_lock };

				const auto it = m_subscriptions.find( msg_type );
				if( it != m_subscriptions.end() )
					{
						// Since v.5.7.4 we have to ask delivery_filter before the
						// delivery attempt.
						const auto delivery_status =
								it->second.must_be_delivered(
										*m_single_consumer,
										message,
										[]( const message_ref_t & m ) -> message_t & {
											return *m;
										} );

						if( delivery_possibility_t::must_be_delivered == delivery_status )
							{
								l( it->second );
							}
						else
							{
								tracer.message_rejected(
										m_single_consumer, delivery_status );
							}
					}
				else
					{
						tracer.no_subscribers();
					}
			}
	};

/*!
 * \since v.5.5.9, v.5.7.4
 *
 * \brief Alias for mpsc_mbox without message delivery tracing and message limits.
 */
using limitful_mpsc_mbox_without_tracing_t =
	mpsc_mbox_template_t<
			msg_tracing_helpers::tracing_disabled_base,
			mpsc_mbox_message_limits_usage_t::use
	>;

/*!
 * \since v.5.5.9, v.5.7.4
 *
 * \brief Alias for mpsc_mbox with message delivery tracing and message limits.
 */
using limitful_mpsc_mbox_with_tracing_t =
	mpsc_mbox_template_t<
			msg_tracing_helpers::tracing_enabled_base,
			mpsc_mbox_message_limits_usage_t::use
	>;

/*!
 * \since v.5.5.9, v.5.7.4
 *
 * \brief Alias for mpsc_mbox without message delivery tracing and without
 * message limits.
 */
using limitless_mpsc_mbox_without_tracing_t =
	mpsc_mbox_template_t<
			msg_tracing_helpers::tracing_disabled_base,
			mpsc_mbox_message_limits_usage_t::dont_use
	>;

/*!
 * \since v.5.5.9, v.5.7.4
 *
 * \brief Alias for mpsc_mbox with message delivery tracing and without message limits.
 */
using limitless_mpsc_mbox_with_tracing_t =
	mpsc_mbox_template_t<
			msg_tracing_helpers::tracing_enabled_base,
			mpsc_mbox_message_limits_usage_t::dont_use
	>;

} /* namespace impl */

} /* namespace so_5 */


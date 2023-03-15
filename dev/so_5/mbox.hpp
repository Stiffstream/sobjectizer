/*
	SObjectizer 5.
*/

/*!
	\file
	\brief Mbox definition.
*/

#pragma once

#include <string>
#include <memory>
#include <typeindex>
#include <utility>

#include <so_5/declspec.hpp>
#include <so_5/compiler_features.hpp>

#include <so_5/exception.hpp>

#include <so_5/wait_indication.hpp>

#include <so_5/mbox_fwd.hpp>
#include <so_5/message.hpp>
#include <so_5/message_sink.hpp>
#include <so_5/mhood.hpp>

namespace so_5
{

/*!
 * \since
 * v.5.5.9
 *
 * \brief Result of checking delivery posibility.
 */
enum class delivery_possibility_t
{
	must_be_delivered,
	no_subscription,
	disabled_by_delivery_filter,
	/*!
	 * The actual message is hidden by an envelope.
	 *
	 * \since
	 * v.5.5.23
	 */
	hidden_by_envelope
};

//
// delivery_filter_t
//
/*!
 * \since
 * v.5.5.5
 *
 * \brief An interface of delivery filter object.
 */
class SO_5_TYPE delivery_filter_t
	{
		// Note: clang-3.9 requires this on Windows platform.
		delivery_filter_t( const delivery_filter_t & ) = delete;
		delivery_filter_t( delivery_filter_t && ) = delete;
		delivery_filter_t & operator=( const delivery_filter_t & ) = delete;
		delivery_filter_t & operator=( delivery_filter_t && ) = delete;
	public :
		delivery_filter_t() = default;
		virtual ~delivery_filter_t() noexcept = default;

		//! Checker for a message instance.
		/*!
		 * \retval true message must be delivered to a receiver.
		 * \retval false message must be descarded.
		 */
		[[nodiscard]]
		virtual bool
		check(
			//! Receiver of the message.
			const abstract_message_sink_t & receiver,
			//! Message itself.
			message_t & msg ) const noexcept = 0;
	};

//
// delivery_filter_unique_ptr_t
//
/*!
 * \since
 * v.5.5.5
 *
 * \brief An alias of unique_ptr for delivery_filter.
 */
using delivery_filter_unique_ptr_t =
	std::unique_ptr< delivery_filter_t >;

namespace low_level_api
{

/*!
 * \brief An implementation of delivery filter represented by lambda-function
 * like object.
 *
 * NOTE. This template is moved into low_level_api namespace in v.5.8.0.
 *
 * \tparam Lambda type of lambda-function or functional object.
 *
 * \since v.5.5.5, v.5.8.0
 */
template< typename Lambda, typename Message >
class lambda_as_filter_t : public delivery_filter_t
	{
		Lambda m_filter;

	public :
		lambda_as_filter_t( Lambda && filter )
			:	m_filter( std::forward< Lambda >( filter ) )
			{}

		bool
		check(
			const abstract_message_sink_t & /*receiver*/,
			message_t & msg ) const noexcept override
			{
				return m_filter(message_payload_type< Message >::payload_reference( msg ));
			}
	};

} /* namespace low_level_api */

//
// mbox_type_t
//
/*!
 * \since
 * v.5.5.3
 *
 * \brief Type of the message box.
 *
 * \note
 * This type is no more marked as deprecated.
 */
enum class mbox_type_t
	{
		//! Mbox is Multi-Producer and Multi-Consumer.
		//! Anyone can send messages to it, there can be many subscribers.
		multi_producer_multi_consumer,
		//! Mbox is Multi-Producer and Single-Consumer.
		//! Anyone can send messages to it, there can be only one subscriber.
		multi_producer_single_consumer
	};

//
// abstract_message_box_t
//

//! Mail box class.
/*!
 * The class serves as an interface for sending and receiving messages.
 *
 * All mboxes can be created via the SObjectizer Environment. References to
 * mboxes are stored and manipulated by so_5::mbox_t objects.
 *
 * \see environment_t::schedule_timer(), environment_t::single_timer().
 */
class SO_5_TYPE abstract_message_box_t : protected atomic_refcounted_t
{
		friend class intrusive_ptr_t< abstract_message_box_t >;

		/*!
		 * It is necessary for for access to do_deliver_message_from_timer().
		 *
		 * \note
		 * Added in v.5.5.18.
		 */
		friend class so_5::impl::mbox_iface_for_timers_t;

		abstract_message_box_t( const abstract_message_box_t & ) = delete;
		abstract_message_box_t( abstract_message_box_t && ) = delete;
		abstract_message_box_t &
		operator=( const abstract_message_box_t & ) = delete;
		abstract_message_box_t &
		operator=( abstract_message_box_t && ) = delete;

	public:
		abstract_message_box_t() = default;
		virtual ~abstract_message_box_t() noexcept = default;

		/*!
		 * \since
		 * v.5.4.0
		 *
		 * \brief Unique ID of this mbox.
		 */
		[[nodiscard]]
		virtual mbox_id_t
		id() const = 0;

		//! Add the message handler.
		virtual void
		subscribe_event_handler(
			//! Message type.
			const std::type_index & type_index,
			//! Subscriber.
			abstract_message_sink_t & subscriber ) = 0;

		//! Remove all message handlers.
		virtual void
		unsubscribe_event_handlers(
			//! Message type.
			const std::type_index & type_index,
			//! Subscriber.
			abstract_message_sink_t & subscriber ) = 0;

		//! Get the mbox name.
		[[nodiscard]]
		virtual std::string
		query_name() const = 0;

		/*!
		 * \since
		 * v.5.5.3
		 *
		 * \brief Get the type of message box.
		 *
		 * \note This method is primarily intended for internal usage.
		 * It is useful sometimes in subscription-related operations
		 * because there is no need to do some actions for MPSC mboxes.
		 */
		[[nodiscard]]
		virtual mbox_type_t
		type() const = 0;

		/*!
		 * \name Comparision.
		 * \{
		 */
		[[nodiscard]]
		bool operator==( const abstract_message_box_t & o ) const noexcept
		{
			return id() == o.id();
		}

		[[nodiscard]]
		bool operator<( const abstract_message_box_t & o ) const noexcept
		{
			return id() < o.id();
		}
		/*!
		 * \}
		 */

		/*!
		 * \since
		 * v.5.5.4
		 *
		 * \brief Deliver message for all subscribers with respect to message
		 * limits.
		 *
		 * A message delivery from timer thread is somewhat different from
		 * an ordinary message delivery. Especially in the case when
		 * target mbox is a message chain. If that message chain is
		 * full and some kind of overflow reaction is specified (like waiting
		 * for some time or throwing an exception) then it can lead to
		 * undesired behaviour of the whole application. To take care about
		 * these cases a new method is introduced.
		 *
		 * \note
		 * Since v.5.6.0 this method is used for deliverance of ordinary
		 * messages/signals and for deliverance of enveloped messages.
		 */
		virtual void
		do_deliver_message(
			//! Can the delivery blocks the current thread?
			message_delivery_mode_t delivery_mode,
			//! Type of the message to deliver.
			const std::type_index & msg_type,
			//! A message instance to be delivered.
			const message_ref_t & message,
			//! Current deep of overlimit reaction recursion.
			unsigned int redirection_deep ) = 0;

		/*!
		 * \name Methods for working with delivery filters.
		 * \{
		 */
		/*!
		 * \since
		 * v.5.5.5
		 *
		 * \brief Set a delivery filter for message type and subscriber.
		 *
		 * \note If there already is a delivery filter for that
		 * (msg_type,subscriber) pair then old delivery filter will
		 * be replaced by new one.
		 */
		virtual void
		set_delivery_filter(
			//! Message type to be filtered.
			const std::type_index & msg_type,
			//! Filter to be set.
			//! A caller must guaranted the validity of this reference.
			const delivery_filter_t & filter,
			//! A subscriber for the message.
			abstract_message_sink_t & subscriber ) = 0;

		/*!
		 * \since
		 * v.5.5.5
		 *
		 * \brief Removes delivery filter for message type and subscriber.
		 */
		virtual void
		drop_delivery_filter(
			const std::type_index & msg_type,
			abstract_message_sink_t & subscriber ) noexcept = 0;
		/*!
		 * \}
		 */

		//! SObjectizer Environment for which the mbox is created.
		/*!
		 * \since
		 * v.5.6.0
		 */
		[[nodiscard]]
		virtual so_5::environment_t &
		environment() const noexcept = 0;
};

//
// as_msink
//
//FIXME: document this!
//FIXME: should mbox be checked to be not-null?
[[nodiscard]]
msink_t SO_5_FUNC
wrap_to_msink(
	const mbox_t & mbox,
	priority_t sink_priority = prio::p0 );

namespace low_level_api {

//! Deliver message.
/*!
 * Mbox takes care about destroying a message object.
 *
 * \attention
 * This function ensures that Message is a classical message
 * with an actual data (e.g. \a msg shouldn't be nullptr).
 *
 * \note
 * This function is a part of low-level SObjectizer's interface.
 * Because of that this function can be removed or changed in some
 * future version without prior notice.
 *
 * \since
 * v.5.6.0
 */
template< class Message >
void
deliver_message(
	//! Can the delivery blocks the current thread?
	message_delivery_mode_t delivery_mode,
	//! Destination for message.
	abstract_message_box_t & target,
	//! Subscription type for that message.
	std::type_index subscription_type,
	//! Message data.
	std::unique_ptr< Message > msg )
	{
		ensure_classical_message< Message >();
		ensure_message_with_actual_data( msg.get() );

		target.do_deliver_message(
			delivery_mode,
			std::move(subscription_type),
			message_ref_t{ msg.release() },
			1u );
	}

//! Deliver message.
/*!
 * This function is necessary for cases when message object
 * is already present as message_ref_t.
 *
 * \note
 * This function is a part of low-level SObjectizer's interface.
 * Because of that this function can be removed or changed in some
 * future version without prior notice.
 *
 * \since
 * v.5.6.0
 */
inline void
deliver_message(
	//! Can the delivery blocks the current thread?
	message_delivery_mode_t delivery_mode,
	//! Destination for message.
	abstract_message_box_t & target,
	//! Subscription type for that message.
	std::type_index subscription_type,
	//! Message data.
	message_ref_t msg )
	{
		target.do_deliver_message(
				delivery_mode,
				std::move(subscription_type),
				std::move(msg),
				1u );
	}

//! Deliver signal.
/*!
 * \attention
 * This function ensures that Message is a type of a signal.
 *
 * \note
 * This function is a part of low-level SObjectizer's interface.
 * Because of that this function can be removed or changed in some
 * future version without prior notice.
 *
 * \since
 * v.5.6.0
 */
template< class Message >
void
deliver_signal(
	//! Can the delivery blocks the current thread?
	message_delivery_mode_t delivery_mode,
	//! Destination for signal.
	abstract_message_box_t & target )
	{
		ensure_signal< Message >();

		target.do_deliver_message(
			delivery_mode,
			message_payload_type< Message >::subscription_type_index(),
			message_ref_t(),
			1u );
	}

} /* namespace low_level_api */

} /* namespace so_5 */


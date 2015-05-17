/*
	SObjectizer 5.
*/

/*!
	\file
	\since v.5.4.0
	\brief A multi-producer/single-consumer mbox definition.
*/

#if !defined( _SO_5__RT__IMPL__MPSC_MBOX_ )
#define _SO_5__RT__IMPL__MPSC_MBOX_

#include <so_5/h/types.hpp>
#include <so_5/h/exception.hpp>

#include <so_5/rt/h/mbox.hpp>
#include <so_5/rt/h/event_queue_proxy.hpp>
#include <so_5/rt/h/message_limit.hpp>

namespace so_5
{

namespace rt
{

namespace impl
{

//
// limitless_mpsc_mbox_t
//

/*!
 * \since v.5.4.0
 * \brief A multi-producer/single-consumer mbox definition.
 *
 * \note Since v.5.5.4 is used for implementation of direct mboxes
 * without controling message limits.
 */
class limitless_mpsc_mbox_t : public abstract_message_box_t
{
	public:
		limitless_mpsc_mbox_t(
			mbox_id_t id,
			agent_t * single_consumer,
			event_queue_proxy_ref_t event_queue );

		virtual ~limitless_mpsc_mbox_t();

		virtual mbox_id_t
		id() const override
			{
				return m_id;
			}

		virtual void
		subscribe_event_handler(
			const std::type_index & msg_type,
			const so_5::rt::message_limit::control_block_t * limit,
			agent_t * subscriber ) override;

		virtual void
		unsubscribe_event_handlers(
			const std::type_index & msg_type,
			agent_t * subscriber ) override;

		virtual std::string
		query_name() const override;

		virtual mbox_type_t
		type() const override
			{
				return mbox_type_t::multi_producer_single_consumer;
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

		/*!
		 * \attention Will throw an exception because delivery
		 * filter is not applicable to MPSC-mboxes.
		 */
		virtual void
		set_delivery_filter(
			const std::type_index & msg_type,
			const delivery_filter_t & filter,
			agent_t & subscriber ) override;

		/*!
		 * \attention Will throw an exception because delivery
		 * filter is not applicable to MPSC-mboxes.
		 */
		virtual void
		drop_delivery_filter(
			const std::type_index & msg_type,
			agent_t & subscriber ) SO_5_NOEXCEPT override;

	protected :
		/*!
		 * \brief ID of this mbox.
		 */
		const mbox_id_t m_id;

		//! The only consumer of this mbox's messages.
		agent_t * m_single_consumer;

		//! Event queue for the single consumer.
		event_queue_proxy_ref_t m_event_queue;
};

//
// limitful_mpsc_mbox_t
//

/*!
 * \since v.5.5.4
 * \brief A multi-producer/single-consumer mbox with message limit
 * control.
 *
 * \attention Stores a reference to message limits storage. Because of that
 * this reference must remains correct till the end of the mbox's lifetime.
 */
class limitful_mpsc_mbox_t : public limitless_mpsc_mbox_t
{
	public:
		limitful_mpsc_mbox_t(
			mbox_id_t id,
			agent_t * single_consumer,
			//! This reference must remains correct till the end of
			//! the mbox's lifetime.
			const so_5::rt::message_limit::impl::info_storage_t & limits_storage,
			event_queue_proxy_ref_t event_queue );

		virtual ~limitful_mpsc_mbox_t();

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

	private :
		const so_5::rt::message_limit::impl::info_storage_t & m_limits;
};

} /* namespace impl */

} /* namespace rt */

} /* namespace so_5 */

#endif

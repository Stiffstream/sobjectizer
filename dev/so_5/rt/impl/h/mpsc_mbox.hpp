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

namespace so_5
{

namespace rt
{

namespace impl
{

//
// mpsc_mbox_t
//

/*!
 * \since v.5.4.0
 * \brief A multi-producer/single-consumer mbox definition.
 */
class mpsc_mbox_t : public abstract_message_box_t
{
	public:
		explicit mpsc_mbox_t(
			mbox_id_t id,
			agent_t * single_consumer,
			event_queue_proxy_ref_t event_queue );

		virtual ~mpsc_mbox_t();

		virtual mbox_id_t
		id() const
			{
				return m_id;
			}

		void
		deliver_message(
			const std::type_index & msg_type,
			const message_ref_t & message_ref ) const;

		virtual void
		deliver_service_request(
			const std::type_index & msg_type,
			const message_ref_t & svc_request_ref ) const;

		virtual void
		subscribe_event_handler(
			const std::type_index & msg_type,
			agent_t * subscriber );

		virtual void
		unsubscribe_event_handlers(
			const std::type_index & msg_type,
			agent_t * subscriber );

		virtual std::string
		query_name() const;

	private:
		/*!
		 * \brief ID of this mbox.
		 */
		const mbox_id_t m_id;

		//! The only consumer of this mbox's messages.
		agent_t * m_single_consumer;

		//! Event queue for the single consumer.
		event_queue_proxy_ref_t m_event_queue;
};

} /* namespace impl */

} /* namespace rt */

} /* namespace so_5 */

#endif

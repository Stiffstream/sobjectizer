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
		id() const
			{
				return m_id;
			}

		void
		deliver_message(
			const std::type_index & type_wrapper,
			const message_ref_t & message_ref ) const override;

		virtual void
		deliver_service_request(
			const std::type_index & type_index,
			const message_ref_t & svc_request_ref ) const override;

		virtual void
		subscribe_event_handler(
			const std::type_index & type_wrapper,
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

	private:
		/*!
		 * \since v.5.4.0
		 * \brief ID of this mbox.
		 */
		const mbox_id_t m_id;

		//! Object lock.
		mutable default_rw_spinlock_t m_lock;

		/*!
		 * \since v.5.4.0
		 * \brief Type of container with subscribers to one message type.
		 */
		typedef std::vector< agent_t * > agent_container_t;

		/*!
		 * \since v.5.4.0
		 * \brief Map from message type to subscribers.
		 */
		typedef std::map< std::type_index, agent_container_t >
				messages_table_t;

		//! Map of subscribers to messages.
		messages_table_t m_subscribers;
};

} /* namespace impl */

} /* namespace rt */

} /* namespace so_5 */

#endif

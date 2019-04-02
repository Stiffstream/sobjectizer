/*
	SObjectizer 5.
*/

/*!
	\file
	\brief A definition of the named local mbox.
*/

#pragma once

#include <so_5/types.hpp>
#include <so_5/exception.hpp>

#include <so_5/mbox.hpp>

#include <so_5/impl/mbox_core.hpp>

namespace so_5
{

namespace impl
{

//
// named_local_mbox_t
//

//! A class for the named local mbox.
/*!
 * \note This class is necessary because there is a difference between
 * reference counting for anonymous and named local mboxes. Named
 * local mboxes should have only one instance inside
 * SObjectizer Environment.
*/
class named_local_mbox_t
	:
		public abstract_message_box_t
{
		friend class impl::mbox_core_t;

		named_local_mbox_t(
			const std::string & name,
			const mbox_t & mbox,
			impl::mbox_core_t & mbox_core );

	public:
		~named_local_mbox_t() override;

		mbox_id_t
		id() const override;

		void
		subscribe_event_handler(
			const std::type_index & type_wrapper,
			const so_5::message_limit::control_block_t * limit,
			agent_t & subscriber ) override;

		void
		unsubscribe_event_handlers(
			const std::type_index & type_wrapper,
			agent_t & subscriber ) override;

		std::string
		query_name() const override;

		mbox_type_t
		type() const override;

		void
		do_deliver_message(
			const std::type_index & msg_type,
			const message_ref_t & message,
			unsigned int overlimit_reaction_deep ) override;

		void
		set_delivery_filter(
			const std::type_index & msg_type,
			const delivery_filter_t & filter,
			agent_t & subscriber ) override;

		void
		drop_delivery_filter(
			const std::type_index & msg_type,
			agent_t & subscriber ) noexcept override;

		environment_t &
		environment() const noexcept override;

	private:
		//! Mbox name.
		const std::string m_name;

		//! An utility for this mbox.
		impl::mbox_core_ref_t m_mbox_core;

		//! Actual mbox.
		mbox_t m_mbox;
};

} /* namespace impl */

} /* namespace so_5 */

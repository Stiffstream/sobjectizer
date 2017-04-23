/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Definition of mbox_iface_for_timers.
 *
 * \since
 * v.5.5.19
 */

#pragma once

#include <so_5/rt/h/mbox.hpp>

namespace so_5 {

namespace rt {

namespace impl {

//
// mbox_iface_for_timers_t
//
/*!
 * \since
 * v.5.5.18
 *
 * \brief Helper class for accessing protected members from mbox interface.
 */
class mbox_iface_for_timers_t
	{
	public :
		mbox_iface_for_timers_t( const mbox_t & mb )
			:	m_mb( *mb ) {}

		inline void
		deliver_message_from_timer(
			//! Type of the message to deliver.
			const std::type_index & msg_type,
			//! A message instance to be delivered.
			const message_ref_t & message )
			{
				m_mb.do_deliver_message_from_timer( msg_type, message );
			}

	private :
		abstract_message_box_t & m_mb;
	};

} /* namespace impl */

} /* namespace rt */

} /* namespace so_5 */



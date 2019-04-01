/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.5.9
 *
 * \file
 * \brief Helper class for accessing private functionality of message-class.
 */

#pragma once

#include <so_5/message.hpp>

namespace so_5 {

namespace impl {

//FIXME: is this class really needed?
//
// internal_message_iface_t
//
/*!
 * \since
 * v.5.5.9
 *
 * \brief A helper class for accessing the functionality of
 * message-class which is specific for SObjectizer internals only.
 */
class internal_message_iface_t
	{
		//! A message instance to work with.
		const message_t & m_msg;

	public :
		//! Initializing constructor.
		internal_message_iface_t( const message_t & msg )
			:	m_msg( msg )
			{}
	};

} /* namespace impl */

} /* namespace so_5 */



/*
	SObjectizer 5.
*/

/*!
	\file
	\since
	v.5.5.1

	\brief A forward declaration of mbox_t.
*/

#pragma once

#include <so_5/atomic_refcounted.hpp>

namespace so_5
{

class abstract_message_box_t;

//
// mbox_t
//
//! Smart reference for the abstract_message_box.
/*!
 * \note Defined as typedef since v.5.2.0
 */
typedef intrusive_ptr_t< abstract_message_box_t > mbox_t;

namespace impl {

class mbox_iface_for_timers_t;

} /* namespace impl */

} /* namespace so_5 */


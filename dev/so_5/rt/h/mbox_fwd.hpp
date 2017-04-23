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

#include <so_5/h/atomic_refcounted.hpp>

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

namespace rt
{

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::mbox_t instead.
 */
using mbox_t = so_5::mbox_t;

// Just for compatibility with previous versions.
/*!
 * \deprecated Obsolete in v.5.5.1. The so_5::mbox_t must be used instead.
 * Will be removed in v.5.6.0.
 */
using mbox_ref_t = so_5::mbox_t;

namespace impl {

class mbox_iface_for_timers_t;

} /* namespace impl */

} /* namespace rt */

} /* namespace so_5 */


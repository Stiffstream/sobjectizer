/*
	SObjectizer 5.
*/

/*!
	\file
	\since v.5.5.1
	\brief A forward declaration of mbox_t.
*/

#pragma once

#include <so_5/h/atomic_refcounted.hpp>

namespace so_5
{

namespace rt
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

// Just for compatibility with previous versions.
/*!
 * \deprecated Obsolete in v.5.5.1. The so_5::rt::mbox_t must be used instead.
 */
typedef mbox_t mbox_ref_t;

} /* namespace rt */

} /* namespace so_5 */


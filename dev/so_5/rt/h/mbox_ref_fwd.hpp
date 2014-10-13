/*
	SObjectizer 5.
*/

/*!
	\file
	\since v.5.5.1
	\brief A forward declaration of mbox_ref_t.
*/

#pragma once

#include <so_5/h/atomic_refcounted.hpp>

namespace so_5
{

namespace rt
{

class mbox_t;

//
// mbox_ref_t
//
//! Smart reference for the mbox_t.
/*!
 * \note Defined as typedef since v.5.2.0
 */
typedef intrusive_ptr_t< mbox_t > mbox_ref_t;

} /* namespace rt */

} /* namespace so_5 */


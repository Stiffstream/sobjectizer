/*
	SObjectizer 5.
*/

/*!
	\file
	\since v.5.2.0
	\brief A forward declaration of agent_ref_t.
*/

#if !defined( _SO_5__RT__AGENT_REF_FWD_HPP_ )
#define _SO_5__RT__AGENT_REF_FWD_HPP_

#include <so_5/h/atomic_refcounted.hpp>

namespace so_5
{

namespace rt
{

class agent_t;

//
// agent_ref_t
//
//! A smart reference to an agent.
/*!
 * \note Defined as typedef since v.5.2.0
 */
typedef intrusive_ptr_t< agent_t > agent_ref_t;

} /* namespace rt */

} /* namespace so_5 */

#endif


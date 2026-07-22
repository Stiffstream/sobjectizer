/*
	SObjectizer 5.
*/

/*!
	\file
	\since
	v.5.2.0

	\brief A forward declaration of agent_ref_t.
*/

#if !defined( SO_5_AGENT_REF_FWD_HPP )
#define SO_5_AGENT_REF_FWD_HPP

#include <so_5/atomic_refcounted.hpp>

#include <so_5/fwd.hpp>

namespace so_5
{

//
// agent_ref_t
//
//! A smart reference to an agent.
/*!
 * \note Defined as typedef since v.5.2.0
 */
using agent_ref_t = intrusive_ptr_t< agent_t >;

} /* namespace so_5 */

#endif


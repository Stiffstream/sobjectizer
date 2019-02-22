/*
	SObjectizer 5.
*/

/*!
	\file
	\brief Interface for the dispatcher binders definition.
*/

#pragma once

#include <so_5/declspec.hpp>
#include <so_5/fwd.hpp>

#include <memory>

namespace so_5
{

//
// disp_binder_t
//

//! Interface for dispatcher binders.
/*!
 * Dispatcher binders are used in the agent registration process to
 * binding of agents to desired dispatchers.
 */
class SO_5_TYPE disp_binder_t
	: private std::enable_shared_from_this< disp_binder_t >
{
		// Note: clang-3.9 requires this on Windows platform.
		disp_binder_t( const disp_binder_t & ) = delete;
		disp_binder_t( disp_binder_t && ) = delete;
		disp_binder_t & operator=( const disp_binder_t & ) = delete;
		disp_binder_t & operator=( disp_binder_t && ) = delete;

	public:
		disp_binder_t() = default;
		virtual ~disp_binder_t() noexcept = default;

		//! Allocate resources in dispatcher for new agent.
		/*!
		 * This method can and should throw on failure.
		 */
		virtual void
		preallocate_resources(
			//! Agent to be bound.
			agent_t & agent ) = 0;

		//! Undo resources allocation.
		/*!
		 * This method will be called after preallocate_resources() if
		 * cooperation can't be registered by some reasons.
		 *
		 * This method can't throw.
		 */
		virtual void
		undo_preallocation(
			//! Agent for that previous preallocate_resources() was called.
			agent_t & agent ) noexcept = 0;

		//! Bind agent to dispatcher.
		/*!
		 * This method will be called after preallocate_resources().
		 *
		 * The dispatcher should use resources created during
		 * preallocate_resources() call. Because of that this method
		 * must not throw.
		 */
		virtual void
		bind(
			//! Agent for that previous preallocate_resources() was called.
			agent_t & agent ) noexcept = 0;

		//! Unbind agent from dispatcher.
		/*!
		 * This method will be called after bind() method.
		 *
		 * This method can't throw.
		 */
		virtual void
		unbind(
			//! Agent for that previous bind() was called.
			agent_t & agent ) noexcept = 0;
};

//! Typedef for the disp_binder smart pointer.
using disp_binder_shptr_t = std::shared_ptr< disp_binder_t >;

#if 0
//FIXME: may be it should stay here for compatibility reasons?
//Because it can be a simple inline-function that calls the corresponding
//method from environment_t.
//
// make_default_disp_binder
//
/*!
 * \brief Create an instance of the default dispatcher binder.
 *
 * \note This function takes into account a possibility to have
 * different types of environment infrastructures (introduced in v.5.5.19)
 * and creates a default dispatcher binder with respect to the
 * actual environment infrastructure type.
 *
 * Usage example:
 * \code
 * so_5::launch( [](so_5::environment_t & env) {
 * 	env.introduce_coop(
 * 		// Agents from that coop will be bound to the default dispatcher.
 * 		so_5::make_default_disp_binder(env),
 * 		[](so_5::coop_t & coop) {
 * 			coop.make_agent<...>(...);
 * 		} );
 * } );
 * \endcode
 *
 * \since
 * v.5.5.19
 */
SO_5_FUNC disp_binder_unique_ptr_t
make_default_disp_binder( environment_t & env );
#endif

} /* namespace so_5 */


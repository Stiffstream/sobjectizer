/*
	SObjectizer 5.
*/

/*!
	\file
	\brief Interface for the dispatcher binders definition.
*/

#if !defined( SO_5_DISP_BINDER_HPP )
#define SO_5_DISP_BINDER_HPP

#include <so_5/declspec.hpp>
#include <so_5/fwd.hpp>

#include <memory>
#include <atomic>

namespace so_5
{

//
// disp_binder_t
//

//! Interface for dispatcher binders.
/*!
 * Dispatcher binders are used in the agent registration process to
 * binding of agents to desired dispatchers.
 *
 * \attention
 * If an implementation of disp_binder_t interface stores smart
 * pointers to agents in methods preallocate_resources() and bind()
 * then it must drop (or reset) these stored smart references in
 * undo_preallocation() and unbind() methods. Otherwise there will
 * be circular references between disp_binder and agents and this
 * will lead to memory leaks and other related problems (for
 * example, destructors for agents/disp_binders won't be called).
 */
class disp_binder_t
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

		//FIXME(v.5.9.0): this method should be transformed into pure virtual
		//method.
		/*!
		 * \brief Does the dispatcher supports C++20 coroutines with suspension
		 * and resumption.
		 *
		 * Usually dispatchers supports C++20 coroutines automatically because
		 * dispatcher do not see them at all. All coroutines related stuff is
		 * packed into ordinary execution demands.
		 *
		 * But sometimes suspension of a coroutine could break the logic of a
		 * dispatcher. For example, so_5::disp::prio_one_thread::strictly_ordered
		 * dispatcher can't suspend the current demand and allows processing
		 * of other demands in the queue because it may break priority handling.
		 *
		 * When agent with C++20 coroutines is being bound to a dispatcher this
		 * method will be called for the disp_binder. If this method returns
		 * `false` then binding will be interrupted by an exception.
		 *
		 * \attention
		 * The default implementation of this method returns `false`. It means
		 * that a dispatcher is considered as not supporting C++20 coroutines.
		 * If you implement a disp_binder for your dispatcher and your dispatcher
		 * has no problems with suspension/resumption then redefine this method
		 * to return `true`.
		 *
		 * \since v.5.8.7
		 */
		[[nodiscard]] virtual
		bool
		does_support_cpp_coro_suspension() const noexcept
		{
			// In SO-5.8 it's assumed that C++20 coroutines are not supported
			// by default.
			return false;
		}
};

//! Typedef for the disp_binder smart pointer.
using disp_binder_shptr_t = std::shared_ptr< disp_binder_t >;

} /* namespace so_5 */

#endif


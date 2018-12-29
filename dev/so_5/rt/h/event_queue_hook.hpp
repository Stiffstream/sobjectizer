/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Interface of event_queue_hook object.
 *
 * \since
 * v.5.5.24
 */

#pragma once

#include <so_5/h/declspec.hpp>
#include <so_5/h/compiler_features.hpp>

#include <so_5/rt/h/event_queue.hpp>
#include <so_5/rt/h/fwd.hpp>

#include <memory>

namespace so_5 {

//
// event_queue_hook_deleter_fnptr_t
//
/*!
 * \brief A type for deleter of event_queue_hook object.
 *
 * \since
 * v.5.5.24
 */
using event_queue_hook_deleter_fnptr_t = void (*)(event_queue_hook_t *) /*SO_5_NOEXCEPT*/;

//
// event_queue_hook_t
//
/*!
 * \brief Interface of event_queue_hook object
 *
 * Since v.5.5.24 there is a new mechanism for customization of SObjectizer:
 * hook for event_queue. Such hooks allows to replace an original event_queue
 * provided by a dispatcher to some custom version of event_queue. That
 * custom version can do some additional stuff, for example, collect some
 * stats about demands.
 *
 * This class describes an interface of event_queue_hook. The interface
 * contains the following methods:
 *
 * - on_bind(). This method will be called by every agent in SObjectizer
 *   when agent is being bound to a dispatcher. This method can be used
 *   for replacing an original event_queue by custom one;
 * - on_unbind(). This method will be called by every agent when agent
 *   is being unbound from the dispatcher. This method should be used
 *   for releasing any resources allocated for a custom queue created or
 *   used in on_bind() method.
 *
 * \since
 * v.5.5.24
 */
class SO_5_TYPE event_queue_hook_t
	{
		// Note: clang-3.9 requires this on Windows platform.
		event_queue_hook_t( const event_queue_hook_t & ) = delete;
		event_queue_hook_t( event_queue_hook_t && ) = delete;
		event_queue_hook_t & operator=( const event_queue_hook_t & ) = delete;
		event_queue_hook_t & operator=( event_queue_hook_t && ) = delete;

	public :
		event_queue_hook_t() = default;
		virtual ~event_queue_hook_t() = default;

		//! A reaction to binding of an agent to some event_queue.
		/*!
		 * This method can return \a original_queue or a pointer to
		 * a different queue. The agent must use the value returned.
		 */
		SO_5_NODISCARD
		virtual event_queue_t *
		on_bind(
			//! An agent that is being bound to some event_queue.
			agent_t * agent,
			//! An event_queue created for that agent by a dispatcher.
			event_queue_t * original_queue ) SO_5_NOEXCEPT = 0;

		//! A reaction to unbinding of an agent from some event_queue.
		/*!
		 * This method must be called by an agent even if the previous
		 * call to on_bind() returned pointer to the original queue.
		 */
		virtual void
		on_unbind(
			//! An agent that is being bound to some event_queue.
			agent_t * agent,
			//! The pointer returned by previous call to on_bind.
			event_queue_t * queue ) SO_5_NOEXCEPT = 0;

		/*!
		 * \brief An implementation of deleter that use operator delete
		 * for destroying object of type event_queue_hook.
		 *
		 * Pointer to this method is intended to be used with
		 * event_queue_hook_unique_ptr_t. For example:
		 * \code
		 * class my_hook final : public so_5::event_queue_hook_t {
		 * 	...
		 * };
		 * ...
		 * so_5::event_queue_hook_unique_ptr_t hook_ptr{
		 * 	new my_hook(...),
		 * 	so_5::event_queue_hook_t::default_deleter };
		 * ...
		 * so_5::launch(
		 * 	[](so_5::environment_t & env) {...},
		 * 	[&](so_5::environment_params_t & params) {
		 * 		params.event_queue_hook(std::move(hook_ptr));
		 * 	});
		 * \endcode
		 */
		static void
		default_deleter( event_queue_hook_t * what ) SO_5_NOEXCEPT;

		/*!
		 * \brief An implementation of no-op deleter.
		 *
		 * This method do nothing. It is intended to be used with
		 * objects those not to be deleted by operator delete. For
		 * example:
		 * \code
		 * class my_hook final : public so_5::event_queue_hook_t {
		 * 	...
		 * };
		 * ...
		 * // Note! This object is created on stack.
		 * my_hook hook{...};
		 * ...
		 * so_5::launch(
		 * 	[](so_5::environment_t & env) {...},
		 * 	[&](so_5::environment_params_t & params) {
		 * 		params.event_queue_hook(
		 * 			so_5::event_queue_hook_unique_ptr_t{
		 * 				&hook,
		 * 				so_5::event_queue_hook_t::noop_deleter});
		 * 	});
		 * \endcode
		 */
		static void
		noop_deleter( event_queue_hook_t * ) SO_5_NOEXCEPT;
	};

//
// event_queue_hook_unique_ptr_t
//
/*!
 * \brief Alias for unique pointer to event_queue_hook.
 *
 * \since
 * v.5.5.24
 */
using event_queue_hook_unique_ptr_t =
		std::unique_ptr<
				event_queue_hook_t,
				event_queue_hook_deleter_fnptr_t >;

//
// make_event_queue_hook
//
/*!
 * \brief Helper function for simplify creation of event_queue_hook
 * object.
 *
 * This helper function allows to write:
 * \code
 * auto hook = so_5::make_event_queue_hook<my_hook>(
 * 	so_5::event_queue_hook_t::default_deleter,
 * 	...); // Some arguments for my_hook's constructor.
 * \endcode
 * instead of:
 * \code
 * so_5::event_queue_hook_unique_ptr_t hook{
 * 	new my_hook{...}, // Some arguments for my_hook's constructor.
 * 	so_5::event_queue_hook_t::default_deleter };
 * \endcode
 *
 * \since
 * v.5.5.24
 */
template< typename Hook_Type, typename... Args >
event_queue_hook_unique_ptr_t
make_event_queue_hook(
	event_queue_hook_deleter_fnptr_t deleter,
	Args && ...args )
	{
		return { new Hook_Type( std::forward<Args>(args)... ), deleter };
	}

//
// make_empty_event_queue_hook_unique_ptr
//
/*!
 * \brief Helper function for creation of empty unique_ptr for event_queue_hook.
 *
 * This function allows to write:
 * \code
 * auto hook = so_5::make_empty_event_queue_hook_unique_ptr();
 * \endcode
 * instead of
 * \code
 * so_5::event_queue_hook_unique_ptr_t hook{nullptr, so_5::event_queue_hook_t::default_deleter};
 * \endcode
 *
 * \since
 * v.5.5.24
 */
SO_5_NODISCARD
inline event_queue_hook_unique_ptr_t
make_empty_event_queue_hook_unique_ptr()
	{
		return { nullptr, &event_queue_hook_t::default_deleter };
	}

} /* namespace so_5 */


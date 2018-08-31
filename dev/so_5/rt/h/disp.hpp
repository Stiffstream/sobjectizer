/*
	SObjectizer 5.
*/

/*!
	\file
	\brief Interface for the dispatcher definition.
*/

#pragma once

#include <memory>
#include <map>
#include <string>

#include <so_5/h/declspec.hpp>
#include <so_5/h/compiler_features.hpp>
#include <so_5/h/ret_code.hpp>

#include <so_5/rt/h/fwd.hpp>

namespace so_5
{

//
// dispatcher_t
//

//! An interface for all dispatchers.
/*!
 * Dispatcher schedules and calls agents' events.
 *
 * Each agent is binded to a dispatcher during the registration.
 * A dispatcher_binder_t object is used for this.
 *
 * Each agent stores its events in the own event queue. When event is
 * stored in the queue an agent informs its dispatcher about it. 
 * The dispatcher should schedule the agent for the event execution on 
 * the agent's working thread context.
 */
class SO_5_TYPE dispatcher_t
{
		// Note: clang-3.9 requires this on Windows platform.
		dispatcher_t( const dispatcher_t & ) = delete;
		dispatcher_t( dispatcher_t && ) = delete;
		dispatcher_t & operator=( const dispatcher_t & ) = delete;
		dispatcher_t & operator=( dispatcher_t && ) = delete;

	public:
		/*! Do nothing. */
		dispatcher_t() = default;
		/*! Do nothing. */
		virtual ~dispatcher_t() SO_5_NOEXCEPT = default;

		/*! Auxiliary method. */
		inline dispatcher_t *
		self_ptr()
		{
			return this;
		}

		//! Launch the dispatcher.
		virtual void
		start(
			//! SObjectizer Environment to work in.
			environment_t & env ) = 0;

		//! Signal about shutdown.
		/*!
		 * Dispatcher must initiate actions for the shutting down of all
		 * working threads. This method shall not block caller until
		 * all threads have beed stopped.
		 */
		virtual void
		shutdown() = 0;

		//! Wait for the full stop of the dispatcher.
		/*!
		 * This method must block the caller until all working threads
		 * have been stopped.
		 */
		virtual void
		wait() = 0;

		/*!
		 * \since
		 * v.5.5.4
		 *
		 * \brief Set basic part of names for data sources for
		 * run-time monitoring.
		 *
		 * \note Do nothing in the base class.
		 */
		virtual void
		set_data_sources_name_base(
			//! Part of name to be used in creation of actual
			//! names of data sources.
			//! Could me empty. In that case the dispatcher will
			//! use some other value for creating names of data sources
			//! (something like `this` pointer).
			const std::string & name );
};

//! Typedef of the dispatcher autopointer.
using dispatcher_unique_ptr_t = std::unique_ptr< dispatcher_t >;

//! Typedef of the dispatcher smart pointer.
using dispatcher_ref_t = std::shared_ptr< dispatcher_t >;

//! Typedef of the map from dispatcher name to a dispather.
using named_dispatcher_map_t = std::map< std::string, dispatcher_ref_t >;

namespace rt
{

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::dispatcher_t instead.
 */
using dispatcher_t = so_5::dispatcher_t;

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::dispatcher_unique_ptr_t
 * instead.
 */
using dispatcher_unique_ptr_t = so_5::dispatcher_unique_ptr_t;

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::dispatcher_ref_t
 * instead.
 */
using dispatcher_ref_t = so_5::dispatcher_ref_t;

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::named_dispatcher_map_t
 * instead.
 */
using named_dispatcher_map_t = so_5::named_dispatcher_map_t;

} /* namespace rt */

} /* namespace so_5 */


/*
	SObjectizer 5.
*/

/*!
	\file
	\brief Interface for the cooperation listener definition.
*/

#pragma once

#include <string>
#include <memory>

#include <so_5/h/declspec.hpp>

#include <so_5/rt/h/fwd.hpp>

namespace so_5
{

//
// coop_listener_t
//

//! Interface for the cooperation listener.
/*!
 * Cooperation listener is intended for observation moments of
 * cooperation registrations and deregistrations.
 *
 * \attention SObjectizer doesn't synchronize calls to the 
 * on_registered() and on_deregistered(). If this is a problem
 * then programmer should take care about the object's thread safety.
 */
class SO_5_TYPE coop_listener_t
{
		// Note: clang-3.9 requires this on Windows platform.
		coop_listener_t( const coop_listener_t & ) = delete;
		coop_listener_t( coop_listener_t && ) = delete;
		coop_listener_t & operator=( const coop_listener_t & ) = delete;
		coop_listener_t & operator=( coop_listener_t && ) = delete;

	public:
		coop_listener_t() = default;
		virtual ~coop_listener_t() SO_5_NOEXCEPT = default;

		//! Hook for the cooperation registration event.
		/*!
		 * Method will be called right after the successful 
		 * cooperation registration.
		 */
		virtual void
		on_registered(
			//! SObjectizer Environment.
			environment_t & so_env,
			//! Cooperation which was registered.
			const std::string & coop_name ) = 0;

		//! Hook for the cooperation deregistration event.
		/*!
		 * Method will be called right after full cooperation deregistration.
		 */
		virtual void
		on_deregistered(
			//! SObjectizer Environment.
			environment_t & so_env,
			//! Cooperation which was registered.
			const std::string & coop_name,
			//! Reason of deregistration.
			const coop_dereg_reason_t & reason ) = 0;
};

//! Typedef for the coop_listener autopointer.
using coop_listener_unique_ptr_t = std::unique_ptr< coop_listener_t >;

namespace rt
{

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::coop_listener_t
 * instead.
 */
using coop_listener_t = so_5::coop_listener_t;

/*!
 * \deprecated Will be removed in v.5.6.0. Use so_5::adhoc_agent_wrapper_t
 * instead.
 */
using coop_listener_unique_ptr_t = so_5::coop_listener_unique_ptr_t;

} /* namespace rt */

} /* namespace so_5 */

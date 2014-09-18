/*
	SObjectizer 5.
*/

/*!
	\file
	\brief Interface for the cooperation listener definition.
*/

#if !defined( _SO_5__RT__COOP_LISTENER_HPP_ )
#define _SO_5__RT__COOP_LISTENER_HPP_

#include <string>
#include <memory>

#include <so_5/h/declspec.hpp>

namespace so_5
{

namespace rt
{

class so_environment_t;
class coop_dereg_reason_t;

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
	public:
		virtual ~coop_listener_t();

		//! Hook for the cooperation registration event.
		/*!
		 * Method will be called right after the successful 
		 * cooperation registration.
		 */
		virtual void
		on_registered(
			//! SObjectizer Environment.
			so_environment_t & so_env,
			//! Cooperation which was registered.
			const std::string & coop_name ) = 0;

		//! Hook for the cooperation deregistration event.
		/*!
		 * Method will be called right after full cooperation deregistration.
		 */
		virtual void
		on_deregistered(
			//! SObjectizer Environment.
			so_environment_t & so_env,
			//! Cooperation which was registered.
			const std::string & coop_name,
			//! Reason of deregistration.
			const coop_dereg_reason_t & reason ) = 0;
};

//! Typedef for the coop_listener autopointer.
typedef std::unique_ptr< coop_listener_t > coop_listener_unique_ptr_t;

} /* namespace rt */

} /* namespace so_5 */

#endif

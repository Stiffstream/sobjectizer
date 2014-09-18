/*
	SObjectizer 5.
*/

/*!
	\file
	\brief Interface for the dispatcher definition.
*/

#if !defined( _SO_5__RT__DISP_HPP_ )
#define _SO_5__RT__DISP_HPP_

#include <memory>
#include <map>
#include <string>

#include <so_5/h/declspec.hpp>
#include <so_5/h/ret_code.hpp>

namespace so_5
{

namespace rt
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
	public:
		/*! Do nothing. */
		dispatcher_t();
		/*! Do nothing. */
		virtual ~dispatcher_t();

		/*! Auxiliary method. */
		inline dispatcher_t *
		self_ptr()
		{
			return this;
		}

		//! Launch the dispatcher.
		virtual void
		start() = 0;

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
};

//! Typedef of the dispatcher autopointer.
typedef std::unique_ptr< dispatcher_t > dispatcher_unique_ptr_t;

//! Typedef of the dispatcher smart pointer.
typedef std::shared_ptr< dispatcher_t >
	dispatcher_ref_t;

//! Typedef of the map from dispatcher name to a dispather.
typedef std::map<
		std::string,
		dispatcher_ref_t >
	named_dispatcher_map_t;

} /* namespace rt */

} /* namespace so_5 */

#endif


/*
	SObjectizer 5.
*/

/*!
	\file
	\brief Functions for creating and binding of the single thread dispatcher.
*/

#if !defined( _SO_5__DISP__ONE_THREAD__PUB_HPP_ )
#define _SO_5__DISP__ONE_THREAD__PUB_HPP_

#include <string>

#include <so_5/h/declspec.hpp>

#include <so_5/rt/h/disp.hpp>
#include <so_5/rt/h/disp_binder.hpp>

namespace so_5
{

namespace disp
{

namespace one_thread
{

//
// private_dispatcher_t
//

/*!
 * \since v.5.5.4
 * \brief An interface for %one_thread private dispatcher.
 */
class SO_5_TYPE private_dispatcher_t : public so_5::atomic_refcounted_t
	{
	public :
		virtual ~private_dispatcher_t();

		//! Create a binder for that private dispatcher.
		virtual so_5::rt::disp_binder_unique_ptr_t
		binder() = 0;
	};

/*!
 * \since v.5.5.4
 * \brief A handle for the %one_thread private dispatcher.
 */
using private_dispatcher_handle_t =
	so_5::intrusive_ptr_t< private_dispatcher_t >;

//! Create a dispatcher.
SO_5_FUNC so_5::rt::dispatcher_unique_ptr_t
create_disp();

/*!
 * \since v.5.5.4
 * \brief Create a private %one_thread dispatcher.
 *
 * \par Usage sample
\code
auto one_thread_disp = so_5::disp::one_thread::create_private_disp();

auto coop = env.create_coop( so_5::autoname,
	// The main dispatcher for that coop will be
	// private one_thread dispatcher.
	one_thread_disp->binder() );
\endcode
 */
SO_5_FUNC private_dispatcher_handle_t
create_private_disp();

//! Create a dispatcher binder object.
SO_5_FUNC so_5::rt::disp_binder_unique_ptr_t
create_disp_binder(
	//! Name of the dispatcher to be bound to.
	const std::string & disp_name );

} /* namespace one_thread */

} /* namespace disp */

} /* namespace so_5 */

#endif


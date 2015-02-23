/*
	SObjectizer 5.
*/

/*!
	\file
	\brief Dispatcher creation and agent binding functions.
*/

#pragma once

#include <string>

#include <so_5/h/declspec.hpp>

#include <so_5/rt/h/disp.hpp>
#include <so_5/rt/h/disp_binder.hpp>

namespace so_5
{

namespace disp
{

namespace active_obj
{

//
// private_dispatcher_t
//

/*!
 * \since v.5.5.4
 * \brief An interface for %active_obj private dispatcher.
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
 * \brief A handle for the %active_obj private dispatcher.
 */
using private_dispatcher_handle_t =
	so_5::intrusive_ptr_t< private_dispatcher_t >;

//! Create a dispatcher.
SO_5_FUNC so_5::rt::dispatcher_unique_ptr_t
create_disp();

/*!
 * \since v.5.5.4
 * \brief Create a private %active_obj dispatcher.
 *
 * \par Usage sample
\code
auto private_disp = so_5::disp::active_obj::create_private_disp();

auto coop = env.create_coop( so_5::autoname,
	// The main dispatcher for that coop will be
	// private active_obj dispatcher.
	private_disp->binder() );
\endcode
 */
SO_5_FUNC private_dispatcher_handle_t
create_private_disp();

//! Create an agent binder.
SO_5_FUNC so_5::rt::disp_binder_unique_ptr_t
create_disp_binder(
	//! Dispatcher name to be bound to.
	const std::string & disp_name );

} /* namespace active_obj */

} /* namespace disp */

} /* namespace so_5 */


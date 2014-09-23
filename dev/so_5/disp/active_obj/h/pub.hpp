/*
	SObjectizer 5.
*/

/*!
	\file
	\brief Dispatcher creation and agent binding functions.
*/

#if !defined( _SO_5__DISP__ACTIVE_OBJ__PUB_HPP_ )
#define _SO_5__DISP__ACTIVE_OBJ__PUB_HPP_

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

//! Create a dispatcher.
SO_5_FUNC so_5::rt::dispatcher_unique_ptr_t
create_disp();

//! Create an agent binder.
SO_5_FUNC so_5::rt::disp_binder_unique_ptr_t
create_disp_binder(
	//! Dispatcher name to be bound to.
	const std::string & disp_name );

} /* namespace active_obj */

} /* namespace disp */

} /* namespace so_5 */

#endif

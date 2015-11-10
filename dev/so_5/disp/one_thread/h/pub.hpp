/*
	SObjectizer 5.
*/

/*!
	\file
	\brief Functions for creating and binding of the single thread dispatcher.
*/

#pragma once

#include <string>

#include <so_5/h/declspec.hpp>

#include <so_5/rt/h/disp.hpp>
#include <so_5/rt/h/disp_binder.hpp>

#include <so_5/disp/one_thread/h/params.hpp>

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

/*!
 * \since v.5.5.10
 * \brief Create an instance of %one_thread dispatcher to be used
 * as named dispatcher.
 *
 * \par Usage sample
	\code
	so_5::launch( &init, []( so_5::rt::environment_params_t & params ) {
			params.add_named_dispatcher( "my_disp",
				so_5::disp::one_thread::create_disp(
					so_5::disp::one_thread::disp_params_t{}.tune_queue_params(
						[]( so_5::disp::one_thread::queue_traits::disp_params_t & p ) {
							p.lock_factory( so_5::disp::one_thread::queue_traits::simple_lock_factory();
						} ) ) );
		} );
	\endcode
 */
SO_5_FUNC so_5::rt::dispatcher_unique_ptr_t
create_disp(
	//! Parameters for the dispatcher.
	disp_params_t params );

//! Create a dispatcher.
inline so_5::rt::dispatcher_unique_ptr_t
create_disp()
	{
		return create_disp( disp_params_t{} );
	}

/*!
 * \since v.5.5.10
 * \brief Create a private %one_thread dispatcher.
 *
 * \par Usage sample
\code
auto one_thread_disp = so_5::disp::one_thread::create_private_disp(
	env,
	"file_handler",
	so_5::disp::one_thread::disp_params_t{}.tune_queue_params(
		[]( so_5::disp::one_thread::queue_traits::queue_params_t & p ) {
			p.lock_factory( so_5::disp::one_thread::queue_traits::simple_lock_factory();
		} ) );
auto coop = env.create_coop( so_5::autoname,
	// The main dispatcher for that coop will be
	// private one_thread dispatcher.
	one_thread_disp->binder() );
\endcode
 */
SO_5_FUNC private_dispatcher_handle_t
create_private_disp(
	//! SObjectizer Environment to work in.
	so_5::rt::environment_t & env,
	//! Value for creating names of data sources for
	//! run-time monitoring.
	const std::string & data_sources_name_base,
	//! Parameters for the dispatcher.
	disp_params_t params );

/*!
 * \since v.5.5.4
 * \brief Create a private %one_thread dispatcher.
 *
 * \par Usage sample
\code
auto one_thread_disp = so_5::disp::one_thread::create_private_disp(
	env,
	"file_handler" );
auto coop = env.create_coop( so_5::autoname,
	// The main dispatcher for that coop will be
	// private one_thread dispatcher.
	one_thread_disp->binder() );
\endcode
 */
inline private_dispatcher_handle_t
create_private_disp(
	//! SObjectizer Environment to work in.
	so_5::rt::environment_t & env,
	//! Value for creating names of data sources for
	//! run-time monitoring.
	const std::string & data_sources_name_base )
	{
		return create_private_disp( env, data_sources_name_base, disp_params_t{} );
	}

/*!
 * \since v.5.5.4
 * \brief Create a private %one_thread dispatcher.
 *
 * \par Usage sample
\code
auto one_thread_disp = so_5::disp::one_thread::create_private_disp( env );

auto coop = env.create_coop( so_5::autoname,
	// The main dispatcher for that coop will be
	// private one_thread dispatcher.
	one_thread_disp->binder() );
\endcode
 */
inline private_dispatcher_handle_t
create_private_disp( so_5::rt::environment_t & env )
	{
		return create_private_disp( env, std::string(), disp_params_t{} );
	}

//! Create a dispatcher binder object.
SO_5_FUNC so_5::rt::disp_binder_unique_ptr_t
create_disp_binder(
	//! Name of the dispatcher to be bound to.
	const std::string & disp_name );

} /* namespace one_thread */

} /* namespace disp */

} /* namespace so_5 */


/*
	SObjectizer 5.
*/

/*!
	\file
	\brief Functions for creating and binding to the active group dispatcher.
*/

#if !defined( _SO_5__DISP__ACTIVE_GROUP__PUB_HPP_ )
#define _SO_5__DISP__ACTIVE_GROUP__PUB_HPP_

#include <string>

#include <so_5/h/declspec.hpp>

#include <so_5/rt/h/disp.hpp>
#include <so_5/rt/h/disp_binder.hpp>

namespace so_5
{

namespace disp
{

namespace active_group
{

//
// private_dispatcher_t
//

/*!
 * \since v.5.5.4
 * \brief An interface for %active_group private dispatcher.
 */
class SO_5_TYPE private_dispatcher_t : public so_5::atomic_refcounted_t
	{
	public :
		virtual ~private_dispatcher_t();

		//! Create a binder for that private dispatcher.
		virtual so_5::rt::disp_binder_unique_ptr_t
		binder(
			//! Active group name to be bound to.
			const std::string & group_name ) = 0;
	};

/*!
 * \since v.5.5.4
 * \brief A handle for the %active_group private dispatcher.
 */
using private_dispatcher_handle_t =
	so_5::intrusive_ptr_t< private_dispatcher_t >;

//! Creates the dispatcher.
SO_5_FUNC so_5::rt::dispatcher_unique_ptr_t
create_disp();

/*!
 * \since v.5.5.4
 * \brief Create a private %active_group dispatcher.
 *
 * \par Usage sample
\code
auto private_disp = so_5::disp::active_group::create_private_disp(
	env,
	"long_req_handlers" );

auto coop = env.create_coop( so_5::autoname,
	// The main dispatcher for that coop will be
	// private active_group dispatcher.
	private_disp->binder( "passive_objects" ) );
\endcode
 */
SO_5_FUNC private_dispatcher_handle_t
create_private_disp(
	//! SObjectizer Environment to work in.
	so_5::rt::environment_t & env,
	//! Value for creating names of data sources for
	//! run-time monitoring.
	const std::string & data_sources_name_base );

/*!
 * \since v.5.5.4
 * \brief Create a private %active_group dispatcher.
 *
 * \par Usage sample
\code
auto private_disp = so_5::disp::active_group::create_private_disp( env );

auto coop = env.create_coop( so_5::autoname,
	// The main dispatcher for that coop will be
	// private active_group dispatcher.
	private_disp->binder( "passive_objects" ) );
\endcode
 */
inline private_dispatcher_handle_t
create_private_disp(
	//! SObjectizer Environment to work in.
	so_5::rt::environment_t & env )
	{
		return create_private_disp( env, std::string() );
	}

//! Creates the dispatcher binder.
SO_5_FUNC so_5::rt::disp_binder_unique_ptr_t
create_disp_binder(
	//! Dispatcher name.
	const std::string & disp_name,
	//! Active group name to be bound to.
	const std::string & group_name );

} /* namespace active_group */

} /* namespace disp */

} /* namespace so_5 */

#endif

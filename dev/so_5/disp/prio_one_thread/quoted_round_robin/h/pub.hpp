/*
	SObjectizer 5.
*/

/*!
 * \since v.5.5.8
 * \file
 * \brief Functions for creating and binding of the single thread dispatcher
 * with priority support (quoted round robin policy).
 */

#pragma once

#include <string>

#include <so_5/h/declspec.hpp>

#include <so_5/rt/h/disp.hpp>
#include <so_5/rt/h/disp_binder.hpp>

#include <so_5/h/priority.hpp>

#include <so_5/disp/prio_one_thread/quoted_round_robin/h/quotes.hpp>

namespace so_5 {

namespace disp {

namespace prio_one_thread {

namespace quoted_round_robin {

//
// private_dispatcher_t
//

/*!
 * \since v.5.5.8
 * \brief An interface for %quoted_round_robin private dispatcher.
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
 * \since v.5.5.8
 * \brief A handle for the %quoted_round_robin private dispatcher.
 */
using private_dispatcher_handle_t =
	so_5::intrusive_ptr_t< private_dispatcher_t >;

//! Create a dispatcher.
SO_5_FUNC so_5::rt::dispatcher_unique_ptr_t
create_disp(
	//! Quotes for every priority.
	const quotes_t & quotes );

/*!
 * \since v.5.5.8
 * \brief Create a private %quoted_round_robin dispatcher.
 *
 * \par Usage sample
\code
using namespace so_5::disp::prio_one_thread::quoted_round_robin;
auto common_thread_disp = create_private_disp(
	env,
	quotes_t{ 75 }.set( so_5::prio::p7, 150 ).set( so_5::prio::p6, 125 ),
	"request_processor" );


auto coop = env.create_coop( so_5::autoname,
	// The main dispatcher for that coop will be
	// private quoted_round_robin dispatcher.
	common_thread_disp->binder() );
\endcode
 */
SO_5_FUNC private_dispatcher_handle_t
create_private_disp(
	//! SObjectizer Environment to work in.
	so_5::rt::environment_t & env,
	//! Quotes for every priority.
	const quotes_t & quotes,
	//! Value for creating names of data sources for
	//! run-time monitoring.
	const std::string & data_sources_name_base );

/*!
 * \since v.5.5.8
 * \brief Create a private %quoted_round_robin dispatcher.
 *
 * \par Usage sample
\code
using namespace so_5::disp::prio_one_thread::quoted_round_robin;
auto common_thread_disp = create_private_disp(
	env,
	quotes_t{ 75 }.set( so_5::prio::p7, 150 ).set( so_5::prio::p6, 125 ) );

auto coop = env.create_coop( so_5::autoname,
	// The main dispatcher for that coop will be
	// private quoted_round_robin dispatcher.
	common_thread_disp->binder() );
\endcode
 */
inline private_dispatcher_handle_t
create_private_disp(
	//! SObjectizer Environment to work in.
	so_5::rt::environment_t & env,
	//! Quotes for every priority.
	const quotes_t & quotes )
	{
		return create_private_disp( env, quotes, std::string() );
	}

//! Create a dispatcher binder object.
SO_5_FUNC so_5::rt::disp_binder_unique_ptr_t
create_disp_binder(
	//! Name of the dispatcher to be bound to.
	const std::string & disp_name );

} /* namespace quoted_round_robin */

} /* namespace prio_one_thread */

} /* namespace disp */

} /* namespace so_5 */


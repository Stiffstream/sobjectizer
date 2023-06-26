/*
	SObjectizer 5.
*/

/*!
 * \file
 * \brief Functions for creating and binding of the single thread dispatcher
 * that provides noexcept guarantee for scheduling evt_finish demand.
 *
 * \since v.5.8.0
 */

#pragma once

#include <so_5/disp/nef_one_thread/params.hpp>

#include <so_5/declspec.hpp>

#include <so_5/disp_binder.hpp>

#include <string>

namespace so_5 {

namespace disp {

namespace nef_one_thread {

namespace impl
{

class dispatcher_handle_maker_t;

} /* namespace impl */

//
// dispatcher_handle_t
//

/*!
 * \brief A handle for %nef_one_thread dispatcher.
 *
 * \since v.5.8.0
 */
class [[nodiscard]] dispatcher_handle_t
	{
		friend class impl::dispatcher_handle_maker_t;

		//! Binder for the dispatcher.
		disp_binder_shptr_t m_binder;

		dispatcher_handle_t( disp_binder_shptr_t binder ) noexcept
			:	m_binder{ std::move(binder) }
			{}

		//! Is this handle empty?
		bool
		empty() const noexcept { return !m_binder; }

	public :
		dispatcher_handle_t() noexcept = default;

		//! Get a binder for that dispatcher.
		[[nodiscard]]
		disp_binder_shptr_t
		binder() const noexcept
			{
				return m_binder;
			}

		//! Is this handle empty?
		operator bool() const noexcept { return empty(); }

		//! Does this handle contain a reference to dispatcher?
		bool
		operator!() const noexcept { return !empty(); }

		//! Drop the content of handle.
		void
		reset() noexcept { m_binder.reset(); }
	};

//
// make_dispatcher
//
/*!
 * \brief Create an instance of %nef_one_thread dispatcher.
 *
 * \par Usage sample
\code
auto my_disp = so_5::disp::nef_one_thread::make_dispatcher(
	env,
	"request_processor",
	so_5::disp::nef_one_thread::disp_params_t{}.tune_queue_params(
		[]( so_5::disp::nef_one_thread::queue_traits::queue_params_t & p ) {
			p.lock_factory( so_5::disp::nef_one_thread::queue_traits::simple_lock_factory() );
		} ) );
auto coop = env.make_coop(
	// The main dispatcher for that coop will be
	// this instance of nef_one_thread dispatcher.
	my_disp.binder() );
\endcode
 *
 * \since v.5.8.0
 */
SO_5_FUNC dispatcher_handle_t
make_dispatcher(
	//! SObjectizer Environment to work in.
	environment_t & env,
	//! Value for creating names of data sources for
	//! run-time monitoring.
	const std::string_view data_sources_name_base,
	//! Parameters for the dispatcher.
	disp_params_t params );

//
// make_dispatcher
//
/*!
 * \brief Create an instance of %nef_one_thread dispatcher.
 *
 * \par Usage sample
\code
auto my_disp = so_5::disp::nef_one_thread::make_dispatcher(
	env,
	"request_processor" );
auto coop = env.make_coop(
	// The main dispatcher for that coop will be
	// this instance of nef_one_thread dispatcher.
	my_disp.binder() );
\endcode
 *
 * \since v.5.8.0
 */
inline dispatcher_handle_t
make_dispatcher(
	//! SObjectizer Environment to work in.
	environment_t & env,
	//! Value for creating names of data sources for
	//! run-time monitoring.
	const std::string_view data_sources_name_base )
	{
		return make_dispatcher( env, data_sources_name_base, disp_params_t{} );
	}

//
// make_dispatcher
//
/*!
 * \brief Create an instance of %nef_one_thread dispatcher.
 *
 * \par Usage sample
\code
auto my_disp = so_5::disp::nef_one_thread::make_dispatcher( env );

auto coop = env.make_coop(
	// The main dispatcher for that coop will be
	// private nef_one_thread dispatcher.
	my_disp.binder() );
\endcode
 *
 * \since v.5.8.0
 */
inline dispatcher_handle_t
make_dispatcher( environment_t & env )
	{
		return make_dispatcher( env, std::string_view{} );
	}

} /* namespace nef_one_thread */

} /* namespace disp */

} /* namespace so_5 */


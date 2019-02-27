/*
	SObjectizer 5.
*/

/*!
	\file
	\brief Dispatcher creation and agent binding functions.
*/

#pragma once

#include <string>

#include <so_5/declspec.hpp>

#include <so_5/disp_binder.hpp>

#include <so_5/disp/mpsc_queue_traits/pub.hpp>

#include <so_5/disp/reuse/work_thread_activity_tracking.hpp>

namespace so_5
{

namespace disp
{

namespace active_obj
{

/*!
 * \brief Alias for namespace with traits of event queue.
 *
 * \since
 * v.5.5.10
 */
namespace queue_traits = so_5::disp::mpsc_queue_traits;

//
// disp_params_t
//
/*!
 * \brief Parameters for active object dispatcher.
 *
 * \since
 * v.5.5.10
 */
class disp_params_t
	:	public so_5::disp::reuse::work_thread_activity_tracking_flag_mixin_t< disp_params_t >
	{
		using activity_tracking_mixin_t = so_5::disp::reuse::
				work_thread_activity_tracking_flag_mixin_t< disp_params_t >;

	public :
		//! Default constructor.
		disp_params_t() = default;

		friend inline void swap( disp_params_t & a, disp_params_t & b )
			{
				swap(
						static_cast< activity_tracking_mixin_t & >(a),
						static_cast< activity_tracking_mixin_t & >(b) );

				swap( a.m_queue_params, b.m_queue_params );
			}

		//! Setter for queue parameters.
		disp_params_t &
		set_queue_params( queue_traits::queue_params_t p )
			{
				m_queue_params = std::move(p);
				return *this;
			}

		//! Tuner for queue parameters.
		/*!
		 * Accepts lambda-function or functional object which tunes
		 * queue parameters.
			\code
			so_5::disp::active_obj::create_private_disp( env,
				"my_active_obj_disp",
				so_5::disp::active_obj::disp_params_t{}.tune_queue_params(
					[]( so_5::disp::active_obj::queue_traits::queue_params_t & p ) {
						p.lock_factory( so_5::disp::active_obj::queue_traits::simple_lock_factory() );
					} ) );
			\endcode
		 */
		template< typename L >
		disp_params_t &
		tune_queue_params( L tunner )
			{
				tunner( m_queue_params );
				return *this;
			}

		//! Getter for queue parameters.
		const queue_traits::queue_params_t &
		queue_params() const
			{
				return m_queue_params;
			}

	private :
		//! Queue parameters.
		queue_traits::queue_params_t m_queue_params;
	};

namespace impl
{

class dispatcher_handle_maker_t;

} /* namespace impl */

//
// dispatcher_handle_t
//

/*!
 * \since
 * v.5.6.0
 *
 * \brief A handle for %active_obj dispatcher.
 */
class dispatcher_handle_t
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

//FIXME: update comment!
/*!
 * \brief Create a private %active_obj dispatcher.
 *
 * \since
 * v.5.5.10
 *
 * \par Usage sample
\code
auto private_disp = so_5::disp::active_obj::create_private_disp(
	env,
	"db_handler",
	// Additional params with specific options for queue's traits.
	so_5::disp::active_obj::disp_params_t{}.tune_queue_params(
		[]( so_5::disp::active_obj::queue_traits::queue_params_t & p ) {
			p.lock_factory( so_5::disp::active_obj::queue_traits::simple_lock_factory() );
		} ) );
auto coop = env.create_coop( so_5::autoname,
	// The main dispatcher for that coop will be
	// private active_obj dispatcher.
	private_disp->binder() );
\endcode
 */
SO_5_FUNC dispatcher_handle_t
make_dispatcher(
	//! SObjectizer Environment to work in.
	environment_t & env,
	//! Value for creating names of data sources for
	//! run-time monitoring.
	const std::string_view data_sources_name_base,
	//! Parameters for dispatcher.
	disp_params_t params );

//FIXME: update comment!
/*!
 * \brief Create a private %active_obj dispatcher.
 *
 * \since
 * v.5.5.4
 *
 * \par Usage sample
\code
auto private_disp = so_5::disp::active_obj::create_private_disp(
	env,
	"db_handler" );

auto coop = env.create_coop( so_5::autoname,
	// The main dispatcher for that coop will be
	// private active_obj dispatcher.
	private_disp->binder() );
\endcode
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

//FIXME: update comment!
/*!
 * \brief Create a private %active_obj dispatcher.
 *
 * \since
 * v.5.5.4
 *
 * \par Usage sample
\code
auto private_disp = so_5::disp::active_obj::create_private_disp( env );

auto coop = env.create_coop( so_5::autoname,
	// The main dispatcher for that coop will be
	// private active_obj dispatcher.
	private_disp->binder() );
\endcode
 */
inline dispatcher_handle_t
make_dispatcher(
	//! SObjectizer Environment to work in.
	environment_t & env )
	{
		return make_dispatcher( env, std::string_view{} );
	}

} /* namespace active_obj */

} /* namespace disp */

} /* namespace so_5 */


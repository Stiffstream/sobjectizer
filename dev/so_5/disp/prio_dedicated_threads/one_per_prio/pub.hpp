/*
	SObjectizer 5.
*/

/*!
 * \file
 * \brief Functions for creating and binding of the dispatcher with
 * dedicated threads per priority.
 *
 * \since
 * v.5.5.8
 */

#pragma once

#include <so_5/declspec.hpp>

#include <so_5/disp_binder.hpp>

#include <so_5/priority.hpp>

#include <so_5/disp/mpsc_queue_traits/pub.hpp>

#include <so_5/disp/reuse/work_thread_activity_tracking.hpp>

#include <string>

namespace so_5 {

namespace disp {

namespace prio_dedicated_threads {

namespace one_per_prio {

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
 * \brief Parameters for a dispatcher.
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
			namespace prio_disp = so_5::disp::prio_dedicated_threads::one_per_prio;
			prio_disp::create_private_disp( env,
				"my_prio_disp",
				prio_disp::disp_params_t{}.tune_queue_params(
					[]( prio_disp::queue_traits::queue_params_t & p ) {
						p.lock_factory( prio_disp::queue_traits::simple_lock_factory() );
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
 * \brief A handle for %prio_dedicated_threads::one_per_prio dispatcher.
 */
class SO_5_NODISCARD dispatcher_handle_t
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
		SO_5_NODISCARD
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
 * \brief Create an instance of %one_per_prio dispatcher.
 *
 * \par Usage sample
\code
using namespace so_5::disp::prio_dedicated_threads::one_per_prio;
auto common_thread_disp = make_dispatcher(
	env,
	"request_processor"
	disp_params_t{}.tune_queue_params(
		[]( queue_traits::queue_params_t & p ) {
			p.lock_factory( queue_traits::simple_lock_factory() );
		} ) );
auto coop = env.create_coop( so_5::autoname,
	// The main dispatcher for that coop will be
	// private strictly_ordered dispatcher.
	common_thread_disp.binder() );
\endcode
 *
 * \since
 * v.5.6.0
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
 * \brief Create an instance of %one_per_prio dispatcher.
 *
 * \par Usage sample
\code
using namespace so_5::disp::prio_dedicated_threads::one_per_prio;
auto common_thread_disp = make_dispatcher( env, "request_processor" );

auto coop = env.create_coop( so_5::autoname,
	// The main dispatcher for that coop will be
	// private strictly_ordered dispatcher.
	common_thread_disp.binder() );
\endcode
 *
 * \since
 * v.5.5.8
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
 * \brief Create an instance of %one_per_prio dispatcher.
 *
 * \par Usage sample
\code
using namespace so_5::disp::prio_dedicated_threads::one_per_prio;
auto common_thread_disp = make_dispatcher( env );

auto coop = env.create_coop( so_5::autoname,
	// The main dispatcher for that coop will be
	// private strictly_ordered dispatcher.
	common_thread_disp.binder() );
\endcode
 *
 * \since
 * v.5.6.0
 */
inline dispatcher_handle_t
make_dispatcher( environment_t & env )
	{
		return make_dispatcher( env, std::string_view{} );
	}

} /* namespace one_per_prio */

} /* namespace prio_dedicated_threads */

} /* namespace disp */

} /* namespace so_5 */


/*
	SObjectizer 5.
*/

/*!
 * \file
 * \brief Functions for creating and binding of the single thread dispatcher
 * with priority support (quoted round robin policy).
 *
 * \since
 * v.5.5.8
 */

#pragma once

#include <so_5/declspec.hpp>

#include <so_5/disp_binder.hpp>

#include <so_5/priority.hpp>

#include <so_5/disp/prio_one_thread/quoted_round_robin/quotes.hpp>

#include <so_5/disp/mpsc_queue_traits/pub.hpp>

#include <so_5/disp/reuse/work_thread_activity_tracking.hpp>

#include <string>

namespace so_5 {

namespace disp {

namespace prio_one_thread {

namespace quoted_round_robin {

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
		disp_params_t() {}

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
			namespace prio_disp = so_5::disp::prio_one_thread::quoted_round_robin;
			auto disp = prio_disp::make_dispatcher( env,
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
 * \brief A handle for %prio_one_thread::strictly_ordered dispatcher.
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
 * \brief Create an instance of %quoted_round_robin dispatcher.
 *
 * \par Usage sample
\code
using namespace so_5::disp::prio_one_thread::quoted_round_robin;
auto common_thread_disp = make_dispatcher(
	env,
	"request_processor",
	quotes_t{ 75 }.set( so_5::prio::p7, 150 ).set( so_5::prio::p6, 125 ),
	disp_params_t{}.tune_queue_params(
		[]( queue_traits::queue_params_t & p ) {
			p.lock_factory( queue_traits::simple_lock_factory() );
		} ) );
auto coop = env.make_coop(
	// The main dispatcher for that coop will be
	// this instance of quoted_round_robin dispatcher.
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
	//! Quotes for every priority.
	const quotes_t & quotes,
	//! Parameters for the dispatcher.
	disp_params_t params );

//
// make_dispatcher
//
/*!
 * \brief Create an instance of %quoted_round_robin dispatcher.
 *
 * \par Usage sample
\code
using namespace so_5::disp::prio_one_thread::quoted_round_robin;
auto common_thread_disp = make_dispatcher(
	env,
	"request_processor",
	quotes_t{ 75 }.set( so_5::prio::p7, 150 ).set( so_5::prio::p6, 125 ) );
auto coop = env.make_coop(
	// The main dispatcher for that coop will be
	// this instance of quoted_round_robin dispatcher.
	common_thread_disp->binder() );
\endcode
 *
 * \since
 * v.5.6.0
 */
inline dispatcher_handle_t
make_dispatcher(
	//! SObjectizer Environment to work in.
	environment_t & env,
	//! Value for creating names of data sources for
	//! run-time monitoring.
	const std::string_view data_sources_name_base,
	//! Quotes for every priority.
	const quotes_t & quotes )
	{
		return make_dispatcher(
				env,
				data_sources_name_base,
				quotes,
				disp_params_t{} );
	}

//
// make_dispatcher
//
/*!
 * \brief Create an instance of %quoted_round_robin dispatcher.
 *
 * \par Usage sample
\code
using namespace so_5::disp::prio_one_thread::quoted_round_robin;
auto common_thread_disp = make_dispatcher(
	env,
	quotes_t{ 75 }.set( so_5::prio::p7, 150 ).set( so_5::prio::p6, 125 ) );

auto coop = env.make_coop(
	// The main dispatcher for that coop will be
	// this instance of quoted_round_robin dispatcher.
	common_thread_disp.binder() );
\endcode
 *
 * \since
 * v.5.6.0
 */
inline dispatcher_handle_t
make_dispatcher(
	//! SObjectizer Environment to work in.
	environment_t & env,
	//! Quotes for every priority.
	const quotes_t & quotes )
	{
		return make_dispatcher( env, std::string_view{}, quotes );
	}

} /* namespace quoted_round_robin */

} /* namespace prio_one_thread */

} /* namespace disp */

} /* namespace so_5 */


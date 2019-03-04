/*
	SObjectizer 5.
*/

/*!
	\file
	\brief Functions for creating and binding to the active group dispatcher.
*/

#pragma once

#include <so_5/declspec.hpp>

#include <so_5/disp_binder.hpp>
#include <so_5/nonempty_name.hpp>

#include <so_5/disp/mpsc_queue_traits/pub.hpp>

#include <so_5/disp/reuse/work_thread_activity_tracking.hpp>

#include <string>
#include <string_view>

namespace so_5
{

namespace disp
{

namespace active_group
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
 * \brief Parameters for active group dispatcher.
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

		friend inline void
		swap( disp_params_t & a, disp_params_t & b ) noexcept
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
			so_5::disp::active_group::create_private_disp( env,
				"my_active_group_disp",
				so_5::disp::active_group::disp_params_t{}.tune_queue_params(
					[]( so_5::disp::active_group::queue_traits::queue_params_t & p ) {
						p.lock_factory( so_5::disp::active_group::queue_traits::simple_lock_factory() );
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

namespace impl {

class actual_dispatcher_iface_t;

//
// basic_dispatcher_iface_t
//
/*!
 * \brief The very basic interface of %active_group dispatcher.
 *
 * This class contains a minimum that is necessary for implementation
 * of dispatcher_handle class.
 *
 * \since
 * v.5.6.0
 */
class basic_dispatcher_iface_t
	:	public std::enable_shared_from_this<actual_dispatcher_iface_t>
	{
	public :
		virtual ~basic_dispatcher_iface_t() noexcept = default;

		SO_5_NODISCARD
		virtual disp_binder_shptr_t
		binder( nonempty_name_t group_name ) = 0;
	};

using basic_dispatcher_iface_shptr_t =
		std::shared_ptr< basic_dispatcher_iface_t >;

class dispatcher_handle_maker_t;

} /* namespace impl */

//
// dispatcher_handle_t
//

/*!
 * \since
 * v.5.6.0
 *
 * \brief A handle for %active_group dispatcher.
 */
class SO_5_NODISCARD dispatcher_handle_t
	{
		friend class impl::dispatcher_handle_maker_t;

		//! A reference to actual implementation of a dispatcher.
		impl::basic_dispatcher_iface_shptr_t m_dispatcher;

		dispatcher_handle_t(
			impl::basic_dispatcher_iface_shptr_t dispatcher ) noexcept
			:	m_dispatcher{ std::move(dispatcher) }
			{}

		//! Is this handle empty?
		bool
		empty() const noexcept { return !m_dispatcher; }

	public :
		dispatcher_handle_t() noexcept = default;

		//! Get a binder for that dispatcher.
		/*!
		 * \attention
		 * An attempt to call this method on empty handle is UB.
		 */
		SO_5_NODISCARD
		disp_binder_shptr_t
		binder(
			//! Name of group for a new agent.
			nonempty_name_t group_name ) const
			{
				return m_dispatcher->binder( std::move(group_name) );
			}

		//! Is this handle empty?
		operator bool() const noexcept { return empty(); }

		//! Does this handle contain a reference to dispatcher?
		bool
		operator!() const noexcept { return !empty(); }

		//! Drop the content of handle.
		void
		reset() noexcept { m_dispatcher.reset(); }
	};

/*!
 * \brief Create an instance of %active_group dispatcher.
 *
 * \par Usage sample
\code
auto disp = so_5::disp::active_group::make_dispatcher(
	env,
	"request_handler",
	// Additional params with specific options for queue's traits.
	so_5::disp::active_group::disp_params_t{}.tune_queue_params(
		[]( so_5::disp::active_group::queue_traits::queue_params_t & p ) {
			p.lock_factory( so_5::disp::active_obj::queue_traits::simple_lock_factory() );
		} ) );
auto coop = env.create_coop( so_5::autoname,
	// The main dispatcher for that coop will be
	// this instance of active_group dispatcher.
	disp.binder( "request_handler" ) );
\endcode
 *
 * \since
 * v.5.6.0
 */
SO_5_FUNC dispatcher_handle_t
make_dispatcher(
	//! SObjectizer Environment to work in.
	so_5::environment_t & env,
	//! Value for creating names of data sources for
	//! run-time monitoring.
	const std::string_view data_sources_name_base,
	//! Parameters for dispatcher.
	disp_params_t params );

/*!
 * \brief Create an instance of %active_group dispatcher.
 *
 * \par Usage sample
\code
auto disp = so_5::disp::active_group::make_dispatcher(
	env,
	"long_req_handlers" );

auto coop = env.create_coop( so_5::autoname,
	// The main dispatcher for that coop will be
	// this instance of active_group dispatcher.
	disp.binder( "passive_objects" ) );
\endcode
 *
 * \since
 * v.5.6.0
 */
inline dispatcher_handle_t
make_dispatcher(
	//! SObjectizer Environment to work in.
	so_5::environment_t & env,
	//! Value for creating names of data sources for
	//! run-time monitoring.
	const std::string_view data_sources_name_base )
	{
		return make_dispatcher( env, data_sources_name_base, disp_params_t{} );
	}

/*!
 * \brief Create an instance of %active_group dispatcher.
 *
 * \par Usage sample
\code
auto disp = so_5::disp::active_group::make_dispatcher( env );

auto coop = env.create_coop( so_5::autoname,
	// The main dispatcher for that coop will be
	// this instance of active_group dispatcher.
	disp.binder( "passive_objects" ) );
\endcode
 *
 * \since
 * v.5.6.0
 */
inline dispatcher_handle_t
make_dispatcher(
	//! SObjectizer Environment to work in.
	so_5::environment_t & env )
	{
		return make_dispatcher( env, std::string_view{} );
	}

} /* namespace active_group */

} /* namespace disp */

} /* namespace so_5 */


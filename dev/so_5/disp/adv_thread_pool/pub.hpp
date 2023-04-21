/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.4.0
 *
 * \file
 * \brief Public interface of advanced thread pool dispatcher.
 */

#pragma once

#include <so_5/declspec.hpp>

#include <so_5/disp_binder.hpp>

#include <so_5/disp/mpmc_queue_traits/pub.hpp>

#include <so_5/disp/reuse/work_thread_activity_tracking.hpp>
#include <so_5/disp/reuse/work_thread_factory_params.hpp>

#include <string_view>
#include <utility>
#include <thread>

namespace so_5
{

namespace disp
{

namespace adv_thread_pool
{

/*!
 * \brief Alias for namespace with traits of event queue.
 *
 * \since
 * v.5.5.11
 */
namespace queue_traits = so_5::disp::mpmc_queue_traits;

//
// disp_params_t
//
/*!
 * \brief Parameters for %adv_thread_pool dispatcher.
 *
 * \since
 * v.5.5.11
 */
class disp_params_t
	:	public so_5::disp::reuse::work_thread_activity_tracking_flag_mixin_t< disp_params_t >
	,	public so_5::disp::reuse::work_thread_factory_mixin_t< disp_params_t >
	{
		using activity_tracking_mixin_t = so_5::disp::reuse::
				work_thread_activity_tracking_flag_mixin_t< disp_params_t >;
		using thread_factory_mixin_t = so_5::disp::reuse::
				work_thread_factory_mixin_t< disp_params_t >;

	public :
		//! Default constructor.
		disp_params_t() {}

		friend inline void
		swap( disp_params_t & a, disp_params_t & b ) noexcept
			{
				using std::swap;

				swap(
						static_cast< activity_tracking_mixin_t & >(a),
						static_cast< activity_tracking_mixin_t & >(b) );

				swap(
						static_cast< work_thread_factory_mixin_t & >(a),
						static_cast< work_thread_factory_mixin_t & >(b) );

				swap( a.m_thread_count, b.m_thread_count );
				swap( a.m_queue_params, b.m_queue_params );
			}

		//! Setter for thread count.
		disp_params_t &
		thread_count( std::size_t count )
			{
				m_thread_count = count;
				return *this;
			}

		//! Getter for thread count.
		std::size_t
		thread_count() const
			{
				return m_thread_count;
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
			using namespace so_5::disp::thread_pool;
			auto disp = make_dispatcher( env,
				"workers_disp",
				disp_params_t{}
					.thread_count( 10 )
					.tune_queue_params(
						[]( queue_traits::queue_params_t & p ) {
							p.lock_factory( queue_traits::simple_lock_factory() );
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
		//! Count of working threads.
		/*!
		 * Value 0 means that actual thread will be detected automatically.
		 */
		std::size_t m_thread_count = { 0 };
		//! Queue parameters.
		queue_traits::queue_params_t m_queue_params;
	};

//
// fifo_t
//
/*!
 * \brief Type of FIFO mechanism for agent's demands.
 *
 * \since
 * v.5.4.0
 */
enum class fifo_t
	{
		//! A FIFO for demands for all agents from the same cooperation.
		/*!
		 * It means that agents from the same cooperation for which this
		 * FIFO mechanism is used will be worked on the same thread.
		 */
		cooperation,
		//! A FIFO for demands only for one agent.
		/*!
		 * It means that FIFO is only supported for the concrete agent.
		 * If several agents from a cooperation have this FIFO type they
		 * will process demands independently and on different threads.
		 */
		individual
	};

//
// bind_params_t
//
/*!
 * \brief Parameters for binding agents to %adv_thread_pool dispatcher.
 *
 * \since
 * v.5.5.11
 */
class bind_params_t
	{
	public :
		//! Set FIFO type.
		bind_params_t &
		fifo( fifo_t v )
			{
				m_fifo = v;
				return *this;
			}

		//! Get FIFO type.
		fifo_t
		query_fifo() const
			{
				return m_fifo;
			}

	private :
		//! FIFO type.
		fifo_t m_fifo = { fifo_t::cooperation };
	};

//
// default_thread_pool_size
//
/*!
 * \brief A helper function for detecting default thread count for
 * thread pool.
 *
 * Returns value of std::thread::hardware_concurrency() or 2 if
 * hardware_concurrency() returns 0.
 *
 * \since
 * v.5.4.0
 */
inline std::size_t
default_thread_pool_size()
	{
		auto c = std::thread::hardware_concurrency();
		if( !c )
			c = 2;

		return c;
	}

namespace impl {

class actual_dispatcher_iface_t;

//
// basic_dispatcher_iface_t
//
/*!
 * \brief The very basic interface of %adv_thread_pool dispatcher.
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

		[[nodiscard]]
		virtual disp_binder_shptr_t
		binder( bind_params_t params ) = 0;
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
 * \brief A handle for %adv_thread_pool dispatcher.
 */
class [[nodiscard]] dispatcher_handle_t
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
		 * Usage example:
		 * \code
		 * using namespace so_5::disp::adv_thread_pool;
		 *
		 * so_5::environment_t & env = ...;
		 * auto disp = make_dispatcher( env );
		 * bind_params_t params;
		 * params.fifo( fifo_t::individual );
		 *
		 * env.introduce_coop( [&]( so_5::coop_t & coop ) {
		 * 	coop.make_agent_with_binder< some_agent_type >(
		 * 		disp.binder( params ),
		 * 		... );
		 *
		 * 	coop.make_agent_with_binder< another_agent_type >(
		 * 		disp.binder( params ),
		 * 		... );
		 *
		 * 	...
		 * } );
		 * \endcode
		 *
		 * \attention
		 * An attempt to call this method on empty handle is UB.
		 */
		[[nodiscard]]
		disp_binder_shptr_t
		binder(
			bind_params_t params ) const
			{
				return m_dispatcher->binder( params );
			}

		//! Create a binder for that dispatcher.
		/*!
		 * This method allows parameters tuning via lambda-function
		 * or other functional objects.
		 *
		 * Usage example:
		 * \code
		 * using namespace so_5::disp::adv_thread_pool;
		 *
		 * so_5::environment_t & env = ...;
		 * env.introduce_coop( [&]( so_5::coop_t & coop ) {
		 * 	coop.make_agent_with_binder< some_agent_type >(
		 * 		// Create dispatcher instance.
		 * 		make_dispatcher( env )
		 * 			// Make and tune binder for that dispatcher.
		 * 			.binder( []( auto & params ) {
		 * 				params.fifo( fifo_t::individual );
		 * 			} ),
		 * 		... );
		 * \endcode
		 *
		 * \attention
		 * An attempt to call this method on empty handle is UB.
		 */
		template< typename Setter >
		[[nodiscard]]
		std::enable_if_t<
				std::is_invocable_v< Setter, bind_params_t& >,
				disp_binder_shptr_t >
		binder(
			//! Function for the parameters tuning.
			Setter && params_setter ) const
			{
				bind_params_t p;
				params_setter( p );

				return this->binder( p );
			}

		//! Get a binder for that dispatcher with default binding params.
		/*!
		 * \attention
		 * An attempt to call this method on empty handle is UB.
		 */
		[[nodiscard]]
		disp_binder_shptr_t
		binder() const
			{
				return this->binder( bind_params_t{} );
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

//
// make_dispatcher
//
/*!
 * \brief Create an instance of %adv_thread_pool dispatcher.
 *
 * \par Usage sample
\code
using namespace so_5::disp::adv_thread_pool;
auto disp = make_dispatcher(
	env,
	"db_workers_pool",
	disp_params_t{}
		.thread_count( 16 )
		.tune_queue_params( []( queue_traits::queue_params_t & params ) {
				params.lock_factory( queue_traits::simple_lock_factory() );
			} ) );
auto coop = env.make_coop(
	// The main dispatcher for that coop will be
	// this instance of adv_thread_pool dispatcher.
	disp.binder() );
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
	disp_params_t disp_params );

//
// make_dispatcher
//
/*!
 * \brief Create an instance of %adv_thread_pool dispatcher.
 *
 * \par Usage sample
\code
auto disp = so_5::disp::adv_thread_pool::make_dispatcher(
	env,
	"req_processors",
	16 );
auto coop = env.make_coop(
	// The main dispatcher for that coop will be
	// this instance of adv_thread_pool dispatcher.
	disp.binder() );
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
	//! Count of working threads.
	std::size_t thread_count )
	{
		return make_dispatcher(
				env,
				data_sources_name_base,
				disp_params_t{}.thread_count( thread_count ) );
	}

/*!
 * \brief Create an instance of %adv_thread_pool dispatcher.
 *
 * \par Usage sample
\code
auto disp = so_5::disp::adv_thread_pool::make_dispatcher( env, 16 );

auto coop = env.make_coop(
	// The main dispatcher for that coop will be
	// this instance of adv_thread_pool dispatcher.
	disp.binder() );
\endcode
 *
 * \since
 * v.5.6.0
 */
inline dispatcher_handle_t
make_dispatcher(
	//! SObjectizer Environment to work in.
	environment_t & env,
	//! Count of working threads.
	std::size_t thread_count )
	{
		return make_dispatcher( env, std::string_view{}, thread_count );
	}

//
// make_dispatcher
//
/*!
 * \brief Create an instance of %adv_thread_pool dispatcher with the default
 * count of work threads.
 *
 * Count of work threads will be detected by default_thread_pool_size()
 * function.
 *
 * \par Usage sample
\code
auto disp = so_5::disp::adv_thread_pool::make_dispatcher( env );

auto coop = env.make_coop(
	// The main dispatcher for that coop will be
	// this instance of adv_thread_pool dispatcher.
	disp.binder() );
\endcode
 *
 * \since
 * v.5.6.0
 */
inline dispatcher_handle_t
make_dispatcher(
	//! SObjectizer Environment to work in.
	environment_t & env )
	{
		return make_dispatcher( env, default_thread_pool_size() );
	}

} /* namespace adv_thread_pool */

} /* namespace disp */

} /* namespace so_5 */


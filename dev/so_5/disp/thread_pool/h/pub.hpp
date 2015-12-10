/*
 * SObjectizer-5
 */

/*!
 * \since v.5.4.0
 * \file
 * \brief Public interface of thread pool dispatcher.
 */

#pragma once

#include <so_5/h/declspec.hpp>

#include <so_5/rt/h/disp_binder.hpp>

#include <so_5/disp/mpmc_queue_traits/h/pub.hpp>

#include <utility>

namespace so_5
{

namespace disp
{

namespace thread_pool
{

/*!
 * \since v.5.5.11
 * \brief Alias for namespace with traits of event queue.
 */
namespace queue_traits = so_5::disp::mpmc_queue_traits;

//
// disp_params_t
//
/*!
 * \since v.5.5.11
 * \brief Parameters for %thread_pool dispatcher.
 */
class disp_params_t
	{
	public :
		//! Default constructor.
		disp_params_t() {}
		//! Copy constructor.
		disp_params_t( const disp_params_t & o )
			:	m_thread_count{ o.m_thread_count }
			,	m_queue_params{ o.m_queue_params }
			{}
		//! Move constructor.
		disp_params_t( disp_params_t && o )
			:	m_thread_count{ std::move(o.m_thread_count) }
			,	m_queue_params{ std::move(o.m_queue_params) }
			{}

		friend inline void swap( disp_params_t & a, disp_params_t & b )
			{
				std::swap( a.m_thread_count, b.m_thread_count );
				swap( a.m_queue_params, b.m_queue_params );
			}

		//! Copy operator.
		disp_params_t & operator=( const disp_params_t & o )
			{
				disp_params_t tmp{ o };
				swap( *this, tmp );
				return *this;
			}
		//! Move operator.
		disp_params_t & operator=( disp_params_t && o )
			{
				disp_params_t tmp{ std::move(o) };
				swap( *this, tmp );
				return *this;
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
			create_private_disp( env,
				"workers_disp",
				disp_params_t{}
					.thread_count( 10 )
					.tune_queue_params(
						[]( queue_traits::params_t & p ) {
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
 * \since v.5.4.0
 * \brief Type of FIFO mechanism for agent's demands.
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
 * \since v.5.5.11
 * \brief Parameters for binding agents to %thread_pool dispatcher.
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

		//! Set maximum count of demands to be processed at once.
		bind_params_t &
		max_demands_at_once( std::size_t v )
			{
				m_max_demands_at_once = v;
				return *this;
			}

		//! Get maximum count of demands to do processed at once.
		std::size_t
		query_max_demands_at_once() const
			{
				return m_max_demands_at_once;
			}

	private :
		//! FIFO type.
		fifo_t m_fifo = { fifo_t::cooperation };

		//! Maximum count of demands to be processed at once.
		std::size_t m_max_demands_at_once = { 4 };
	};

//
// params_t
//
/*!
 * \since v.5.4.0
 * \brief Alias for bind_params.
 * \deprecated Since v.5.5.11 bind_params_t must be used instead.
 */
using params_t = bind_params_t;

//
// default_thread_pool_size
//
/*!
 * \since v.5.4.0
 * \brief A helper function for detecting default thread count for
 * thread pool.
 */
inline std::size_t
default_thread_pool_size()
	{
		auto c = std::thread::hardware_concurrency();
		if( !c )
			c = 2;

		return c;
	}

//
// private_dispatcher_t
//

/*!
 * \since v.5.5.4
 * \brief An interface for %thread_pool private dispatcher.
 */
class SO_5_TYPE private_dispatcher_t : public so_5::atomic_refcounted_t
	{
	public :
		virtual ~private_dispatcher_t();

		//! Create a binder for that private dispatcher.
		virtual disp_binder_unique_ptr_t
		binder(
			//! Binding parameters for the agent.
			const bind_params_t & params ) = 0;

		//! Create a binder for that private dispatcher.
		/*!
		 * This method allows parameters tuning via lambda-function
		 * or other functional objects.
		 */
		template< typename SETTER >
		inline disp_binder_unique_ptr_t
		binder(
			//! Function for the parameters tuning.
			SETTER params_setter )
			{
				bind_params_t p;
				params_setter( p );

				return this->binder( p );
			}
	};

/*!
 * \since v.5.5.4
 * \brief A handle for the %thread_pool private dispatcher.
 */
using private_dispatcher_handle_t =
	so_5::intrusive_ptr_t< private_dispatcher_t >;

//
// create_disp
//
/*!
 * \since v.5.5.11
 * \brief Create thread pool dispatcher.
 *
 * \par Usage sample
\code
so_5::launch( []( so_5::environment_t & env ) {...},
	[]( so_5::environment_params_t & env_params ) {
		using namespace so_5::disp::thread_pool;
		env_params.add_named_dispatcher( create_disp( 
			disp_params_t{}
				.thread_count( 16 )
				.tune_queue_params( queue_traits::params_t & params ) {
						params.lock_factory( queue_traits::simple_lock_factory() );
					} ) );
	} );
\endcode
 */
SO_5_FUNC dispatcher_unique_ptr_t
create_disp(
	//! Parameters for the dispatcher.
	disp_params_t params );

//
// create_disp
//
/*!
 * \since v.5.4.0
 * \brief Create thread pool dispatcher.
 */
inline dispatcher_unique_ptr_t
create_disp(
	//! Count of working threads.
	std::size_t thread_count )
	{
		return create_disp( disp_params_t{}.thread_count( thread_count ) );
	}

//
// create_disp
//
/*!
 * \since v.5.4.0
 * \brief Create thread pool dispatcher.
 *
 * Size of pool is detected automatically.
 */
inline dispatcher_unique_ptr_t
create_disp()
	{
		return create_disp( default_thread_pool_size() );
	}

//
// create_private_disp
//
/*!
 * \since v.5.5.11
 * \brief Create a private %thread_pool dispatcher.
 *
 * \par Usage sample
\code
using namespace so_5::disp::thread_pool;
auto private_disp = create_private_disp(
	env,
	disp_params_t{}
		.thread_count( 16 )
		.tune_queue_params( queue_traits::params_t & params ) {
				params.lock_factory( queue_traits::simple_lock_factory() );
			} ),
	"db_workers_pool" );
auto coop = env.create_coop( so_5::autoname,
	// The main dispatcher for that coop will be
	// private thread_pool dispatcher.
	private_disp->binder( bind_params_t{} ) );
\endcode
 */
SO_5_FUNC private_dispatcher_handle_t
create_private_disp(
	//! SObjectizer Environment to work in.
	environment_t & env,
	//! Parameters for the dispatcher.
	disp_params_t disp_params,
	//! Value for creating names of data sources for
	//! run-time monitoring.
	const std::string & data_sources_name_base );

//
// create_private_disp
//
/*!
 * \since v.5.5.4
 * \brief Create a private %thread_pool dispatcher.
 *
 * \par Usage sample
\code
auto private_disp = so_5::disp::thread_pool::create_private_disp(
	env,
	16,
	"db_workers_pool" );
auto coop = env.create_coop( so_5::autoname,
	// The main dispatcher for that coop will be
	// private thread_pool dispatcher.
	private_disp->binder( so_5::disp::thread_pool::bind_params_t{} ) );
\endcode
 */
inline private_dispatcher_handle_t
create_private_disp(
	//! SObjectizer Environment to work in.
	environment_t & env,
	//! Count of working threads.
	std::size_t thread_count,
	//! Value for creating names of data sources for
	//! run-time monitoring.
	const std::string & data_sources_name_base )
	{
		return create_private_disp(
				env,
				disp_params_t{}.thread_count( thread_count ),
				data_sources_name_base );
	}

/*!
 * \since v.5.5.4
 * \brief Create a private %thread_pool dispatcher.
 *
 * \par Usage sample
\code
auto private_disp = so_5::disp::thread_pool::create_private_disp( env, 16 );

auto coop = env.create_coop( so_5::autoname,
	// The main dispatcher for that coop will be
	// private thread_pool dispatcher.
	private_disp->binder( so_5::disp::thread_pool::bind_params_t{} ) );
\endcode
 */
inline private_dispatcher_handle_t
create_private_disp(
	//! SObjectizer Environment to work in.
	environment_t & env,
	//! Count of working threads.
	std::size_t thread_count )
	{
		return create_private_disp( env, thread_count, std::string() );
	}

//
// create_private_disp
//
/*!
 * \since v.5.5.4
 * \brief Create a private %thread_pool dispatcher with the default
 * count of working threads.
 *
 * \par Usage sample
\code
auto private_disp = so_5::disp::thread_pool::create_private_disp( env );

auto coop = env.create_coop( so_5::autoname,
	// The main dispatcher for that coop will be
	// private thread_pool dispatcher.
	private_disp->binder( so_5::disp::thread_pool::bind_params_t{} ) );
\endcode
 */
inline private_dispatcher_handle_t
create_private_disp(
	//! SObjectizer Environment to work in.
	environment_t & env )
	{
		return create_private_disp(
				env,
				default_thread_pool_size(),
				std::string() );
	}

//
// create_disp_binder
//
/*!
 * \since v.5.4.0
 * \brief Create dispatcher binder for thread pool dispatcher.
 */
SO_5_FUNC disp_binder_unique_ptr_t
create_disp_binder(
	//! Name of the dispatcher.
	std::string disp_name,
	//! Parameters for binding.
	const bind_params_t & params );

/*!
 * \since v.5.4.0
 * \brief Create dispatcher binder for thread pool dispatcher.
 *
 * Usage example:
\code
create_disp_binder( "tpool",
	[]( so_5::disp::thread_pool::bind_params_t & p ) {
		p.fifo( so_5::disp::thread_pool::fifo_t::individual );
		p.max_demands_at_once( 128 );
	} );
\endcode
 */
template< typename SETTER >
inline disp_binder_unique_ptr_t
create_disp_binder(
	//! Name of the dispatcher.
	std::string disp_name,
	//! Function for setting the binding's params.
	SETTER params_setter )
	{
		bind_params_t params;
		params_setter( params );

		return create_disp_binder( std::move(disp_name), params );
	}

} /* namespace thread_pool */

} /* namespace disp */

} /* namespace so_5 */


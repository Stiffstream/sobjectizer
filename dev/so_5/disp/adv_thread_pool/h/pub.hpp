/*
 * SObjectizer-5
 */

/*!
 * \since v.5.4.0
 * \file
 * \brief Public interface of advanced thread pool dispatcher.
 */

#pragma once

#include <functional>

#include <so_5/h/declspec.hpp>

#include <so_5/rt/h/disp_binder.hpp>

namespace so_5
{

namespace disp
{

namespace adv_thread_pool
{

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
// params_t
//
/*!
 * \since v.5.4.0
 * \brief Parameters for advanced thread pool dispatcher.
 */
class SO_5_TYPE params_t
	{
	public :
		//! Default constructor.
		/*!
		 * Sets FIFO to fifo_t::cooperation.
		 */
		params_t();

		//! Set FIFO type.
		params_t &
		fifo( fifo_t v );

		//! Get FIFO type.
		fifo_t
		query_fifo() const;

	private :
		//! FIFO type.
		fifo_t m_fifo;
	};

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
 * \brief An interface for %adv_thread_pool private dispatcher.
 */
class SO_5_TYPE private_dispatcher_t : public so_5::atomic_refcounted_t
	{
	public :
		virtual ~private_dispatcher_t();

		//! Create a binder for that private dispatcher.
		virtual so_5::rt::disp_binder_unique_ptr_t
		binder(
			//! Binding parameters for the agent.
			const params_t & params ) = 0;

		//! Create a binder for that private dispatcher.
		/*!
		 * This method allows parameters tuning via lambda-function
		 * or other functional objects.
		 */
		inline so_5::rt::disp_binder_unique_ptr_t
		binder(
			//! Function for the parameters tuning.
			std::function< void(params_t &) > params_setter )
			{
				params_t p;
				params_setter( p );

				return this->binder( p );
			}
	};

/*!
 * \since v.5.5.4
 * \brief A handle for the %adv_thread_pool private dispatcher.
 */
using private_dispatcher_handle_t =
	so_5::intrusive_ptr_t< private_dispatcher_t >;

//
// create_disp
//
/*!
 * \since v.5.4.0
 * \brief Create thread pool dispatcher.
 */
SO_5_FUNC so_5::rt::dispatcher_unique_ptr_t
create_disp(
	//! Count of working threads.
	std::size_t thread_count );

//
// create_disp
//
/*!
 * \since v.5.4.0
 * \brief Create thread pool dispatcher.
 *
 * Size of pool is detected automatically.
 */
inline so_5::rt::dispatcher_unique_ptr_t
create_disp()
	{
		return create_disp( default_thread_pool_size() );
	}

//
// create_private_disp
//
/*!
 * \since v.5.5.4
 * \brief Create a private %adv_thread_pool dispatcher.
 *
 * \par Usage sample
\code
auto private_disp = so_5::disp::adv_thread_pool::create_private_disp(
	env,
	16,
	"req_processors" );

auto coop = env.create_coop( so_5::autoname,
	// The main dispatcher for that coop will be
	// private adv_thread_pool dispatcher.
	private_disp->binder( so_5::disp::adv_thread_pool::params_t{} ) );
\endcode
 */
SO_5_FUNC private_dispatcher_handle_t
create_private_disp(
	//! SObjectizer Environment to work in.
	so_5::rt::environment_t & env,
	//! Count of working threads.
	std::size_t thread_count,
	//! Value for creating names of data sources for
	//! run-time monitoring.
	const std::string & data_sources_name_base );

/*!
 * \since v.5.5.4
 * \brief Create a private %adv_thread_pool dispatcher.
 *
 * \par Usage sample
\code
auto private_disp = so_5::disp::adv_thread_pool::create_private_disp( env, 16 );

auto coop = env.create_coop( so_5::autoname,
	// The main dispatcher for that coop will be
	// private adv_thread_pool dispatcher.
	private_disp->binder( so_5::disp::adv_thread_pool::params_t{} ) );
\endcode
 */
inline private_dispatcher_handle_t
create_private_disp(
	//! SObjectizer Environment to work in.
	so_5::rt::environment_t & env,
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
 * \brief Create a private %adv_thread_pool dispatcher with the default
 * count of working threads.
 *
 * \par Usage sample
\code
auto private_disp = so_5::disp::adv_thread_pool::create_private_disp( env );

auto coop = env.create_coop( so_5::autoname,
	// The main dispatcher for that coop will be
	// private adv_thread_pool dispatcher.
	private_disp->binder( so_5::disp::adv_thread_pool::params_t{} ) );
\endcode
 */
inline private_dispatcher_handle_t
create_private_disp(
	//! SObjectizer Environment to work in.
	so_5::rt::environment_t & env )
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
SO_5_FUNC so_5::rt::disp_binder_unique_ptr_t
create_disp_binder(
	//! Name of the dispatcher.
	std::string disp_name,
	//! Parameters for binding.
	const params_t & params );

/*!
 * \since v.5.4.0
 * \brief Create dispatcher binder for thread pool dispatcher.
 *
 * Usage example:
\code
create_disp_binder( "tpool",
	[]( so_5::disp::adv_thread_pool::params_t & p ) {
		p.fifo( so_5::disp::thread_pool::fifo_t::individual );
	} );
\endcode
 */
inline so_5::rt::disp_binder_unique_ptr_t
create_disp_binder(
	//! Name of the dispatcher.
	std::string disp_name,
	//! Function for setting the binding's params.
	std::function< void(params_t &) > params_setter )
	{
		params_t params;
		params_setter( params );

		return create_disp_binder( std::move(disp_name), params );
	}

} /* namespace adv_thread_pool */

} /* namespace disp */

} /* namespace so_5 */


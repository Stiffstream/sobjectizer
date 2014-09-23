/*
 * SObjectizer-5
 */

/*!
 * \since v.5.4.0
 * \file
 * \brief Public interface of thread pool dispatcher.
 */

#pragma once

#include <functional>

#include <so_5/h/declspec.hpp>

#include <so_5/rt/h/disp_binder.hpp>

namespace so_5
{

namespace disp
{

namespace thread_pool
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
 * \brief Parameters for thread pool dispatcher.
 */
class SO_5_TYPE params_t
	{
	public :
		//! Default constructor.
		/*!
		 * Sets FIFO to fifo_t::cooperation and
		 * max_demands_at_once to 4.
		 */
		params_t();

		//! Set FIFO type.
		params_t &
		fifo( fifo_t v );

		//! Get FIFO type.
		fifo_t
		query_fifo() const;

		//! Set maximum count of demands to be processed at once.
		params_t &
		max_demands_at_once( std::size_t v );

		//! Get maximum count of demands to do processed at once.
		std::size_t
		query_max_demands_at_once() const;

	private :
		//! FIFO type.
		fifo_t m_fifo;

		//! Maximum count of demands to be processed at once.
		std::size_t m_max_demands_at_once;
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
	[]( so_5::disp::thread_pool::params_t & p ) {
		p.fifo( so_5::disp::thread_pool::fifo_t::individual );
		p.max_demands_at_once( 128 );
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

} /* namespace thread_pool */

} /* namespace disp */

} /* namespace so_5 */


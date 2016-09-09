/*
	SObjectizer 5.
*/

/*!
 * \file
 * \brief Parameters for %one_thread dispatcher.
 *
 * \note Parameters are described in separate header file to avoid
 * header file cycles (parameters needs for environment_params_t declared
 * in environment.hpp, which can be included in dispatcher definition file).
 *
 * \since
 * v.5.5.10
 */

#pragma once

#include <so_5/disp/mpsc_queue_traits/h/pub.hpp>

#include <so_5/disp/reuse/h/work_thread_activity_tracking.hpp>

namespace so_5
{

namespace disp
{

namespace one_thread
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
 * \brief Parameters for one thread dispatcher.
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
		//! Copy constructor.
		disp_params_t( const disp_params_t & o )
			:	activity_tracking_mixin_t( o )
			,	m_queue_params{ o.m_queue_params }
			{}
		//! Move constructor.
		disp_params_t( disp_params_t && o )
			:	activity_tracking_mixin_t( std::move(o) )
			,	m_queue_params{ std::move(o.m_queue_params) }
			{}

		friend inline void swap( disp_params_t & a, disp_params_t & b )
			{
				swap(
						static_cast< activity_tracking_mixin_t & >(a),
						static_cast< activity_tracking_mixin_t & >(b) );
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
			so_5::disp::one_thread::create_private_disp( env,
				"my_one_thread_disp",
				so_5::disp::one_thread::disp_params_t{}.tune_queue_params(
					[]( so_5::disp::one_thread::queue_traits::queue_params_t & p ) {
						p.lock_factory( so_5::disp::one_thread::queue_traits::simple_lock_factory() );
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

//
// params_t
//
/*!
 * \brief Old alias for disp_params for compatibility with previous versions.
 * \deprecated Use disp_params_t instead.
 */
using params_t = disp_params_t;

} /* namespace one_thread */

} /* namespace disp */

} /* namespace so_5 */


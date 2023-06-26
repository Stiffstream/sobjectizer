/*
	SObjectizer 5.
*/

/*!
 * \file
 * \brief Parameters for %nef_one_thread dispatcher.
 *
 * \since v.5.8.0
 */

#pragma once

#include <so_5/declspec.hpp>

#include <so_5/disp/mpsc_queue_traits/pub.hpp>

#include <so_5/disp/reuse/work_thread_activity_tracking.hpp>
#include <so_5/disp/reuse/work_thread_factory_params.hpp>

namespace so_5 {

namespace disp {

namespace nef_one_thread {

/*!
 * \brief Alias for namespace with traits of event queue.
 *
 * \since v.5.8.0
 */
namespace queue_traits = so_5::disp::mpsc_queue_traits;

//
// disp_params_t
//
/*!
 * \brief Parameters for a dispatcher.
 *
 * \since v.5.8.0
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
		disp_params_t() = default;

		friend inline void
		swap( disp_params_t & a, disp_params_t & b ) noexcept
			{
				swap(
						static_cast< activity_tracking_mixin_t & >(a),
						static_cast< activity_tracking_mixin_t & >(b) );

				swap(
						static_cast< work_thread_factory_mixin_t & >(a),
						static_cast< work_thread_factory_mixin_t & >(b) );

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
			namespace nef_disp = so_5::disp::nef_one_thread;
			auto disp = nef_disp::make_dispatcher( env,
				"my_nef_disp",
				nef_disp::disp_params_t{}.tune_queue_params(
					[]( nef_disp::queue_traits::queue_params_t & p ) {
						p.lock_factory( nef_disp::queue_traits::simple_lock_factory() );
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
		queue_params() const noexcept
			{
				return m_queue_params;
			}

	private :
		//! Queue parameters.
		queue_traits::queue_params_t m_queue_params;
	};

} /* namespace nef_one_thread */

} /* namespace disp */

} /* namespace so_5 */


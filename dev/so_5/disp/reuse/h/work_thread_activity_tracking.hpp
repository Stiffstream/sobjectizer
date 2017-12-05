/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Various helpers to work with thread activity tracking stuff.
 *
 * \since
 * v.5.5.18
 */

#pragma once

#include <so_5/h/types.hpp>

namespace so_5 {

namespace disp {

namespace reuse {

/*!
 * \brief Mixin with thread activity tracking flag.
 *
 * Indended to be used as mixin for various disp_params_t classes.
 *
 * \since
 * v.5.5.18
 */
template< typename Params >
class work_thread_activity_tracking_flag_mixin_t
	{
		work_thread_activity_tracking_t m_flag{
				work_thread_activity_tracking_t::unspecified };

	public :
		//! Getter for work thread activity tracking.
		work_thread_activity_tracking_t
		work_thread_activity_tracking() const
			{
				return m_flag;
			}

		friend inline void swap(
				work_thread_activity_tracking_flag_mixin_t & a,
				work_thread_activity_tracking_flag_mixin_t & b )
			{
				std::swap( a.m_flag, b.m_flag );
			}

		//! Setter for work thread activity tracking.
		Params &
		work_thread_activity_tracking(
			work_thread_activity_tracking_t v )
			{
				m_flag = v;
				return static_cast< Params & >(*this);
			}

		//! Helper for turning work thread activity tracking on.
		/*!
		 * \since
		 * v.5.5.18
		 */
		Params &
		turn_work_thread_activity_tracking_on()
			{
				return work_thread_activity_tracking(
						work_thread_activity_tracking_t::on );
			}

		//! Helper for turning work thread activity tracking off.
		/*!
		 * \since
		 * v.5.5.18
		 */
		Params &
		turn_work_thread_activity_tracking_off()
			{
				return work_thread_activity_tracking(
						work_thread_activity_tracking_t::off );
			}
	};

} /* namespace reuse */

} /* namespace disp */

} /* namespace so_5 */


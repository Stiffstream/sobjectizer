/*
 * SObjectizer-5
 */

/*!
 * \file
 * \since
 * v.5.5.4
 *
 * \brief A public interface for control SObjectizer monitoring options.
 */

#pragma once

#include <so_5/h/declspec.hpp>

#include <so_5/rt/h/mbox.hpp>

#include <chrono>

namespace so_5
{

namespace stats
{

/*!
 * \since
 * v.5.5.4
 *
 * \brief A public interface for control SObjectizer monitoring options.
 */
class SO_5_TYPE controller_t
	{
	protected :
		// Note: clang-3.9 requires this on Windows platform.
		controller_t( const controller_t & ) = delete;
		controller_t( controller_t && ) = delete;
		controller_t & operator=( const controller_t & ) = delete;
		controller_t & operator=( controller_t && ) = delete;

		controller_t() = default;
		~controller_t() = default;

	public :
		//! Get the mbox for receiving monitoring information.
		virtual const mbox_t &
		mbox() const = 0;

		//! Turn the monitoring on.
		virtual void
		turn_on() = 0;

		//! Turn the monitoring off.
		virtual void
		turn_off() = 0;

		//! Set distribution period.
		/*!
		 * \return Old distribution period value.
		 */
		virtual std::chrono::steady_clock::duration
		set_distribution_period(
			//! New period value.
			std::chrono::steady_clock::duration period ) = 0;

	protected :
		/*!
		 * \brief Default distribution period.
		 *
		 * \since
		 * v.5.5.19
		 */
		inline static std::chrono::steady_clock::duration
		default_distribution_period()
			{
				return std::chrono::seconds{ 2 };
			}
	};

} /* namespace stats */

} /* namespace so_5 */



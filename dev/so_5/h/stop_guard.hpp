/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief An iterface of stop_guard.
 *
 * \since
 * v.5.5.19.2
 */

#pragma once

#include <so_5/h/declspec.hpp>
#include <so_5/h/compiler_features.hpp>

#include <memory>

namespace so_5 {

//
// stop_guard
//
/*!
 * \brief An interface of stop_guard entity.
 *
 * All implementations of stop_guards must implement this interface.
 *
 * \since
 * v.5.5.19.2
 */
class SO_5_FUNC stop_guard_t
	{
	public :
		//! Type for result of setting up a new stop_guard.
		enum class setup_result_t
			{
				//! New stop_guard has be set successfully.
				ok,
				//! New stop_guard has not been set because stop operation
				//! is already in progress.
				stop_already_in_progress
			};

		//! What to do if attempt to setup a new stop_guard is performed
		//! when the stop operation is in progress.
		enum class what_if_stop_in_progress_t
			{
				//! An exception must be thrown.
				throw_exception,
				//! Return negative setup result.
				return_negative_result
			};

		stop_guard_t() = default;
		stop_guard_t( const stop_guard_t & ) = delete;
		stop_guard_t( stop_guard_t && ) = delete;

		virtual ~stop_guard_t() SO_5_NOEXCEPT = default;

		stop_guard_t & operator=( const stop_guard_t & ) = delete;
		stop_guard_t & operator=( stop_guard_t && ) = delete;

		//! Perform stop-related actions.
		/*!
		 * This method will be called by SObjectizer Environment
		 * when an user called environment_t::stop().
		 *
		 * Stop_guard must start stop-related activities. When all these
		 * activities will be finished the stop_guard must remove itself
		 * from the list of stop_guards.
		 */
		virtual void
		stop() SO_5_NOEXCEPT = 0;
	};

//
// stop_guard_shptr_t
//
/*!
 * \brief An alias of shared_ptr for stop_guard.
 *
 * \since
 * v.5.5.19.2
 */
using stop_guard_shptr_t = std::shared_ptr< stop_guard_t >;

} /* namespace so_5 */


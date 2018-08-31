/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Interface of manager for default locks for event queues.
 *
 * \since
 * v.5.5.18
 */

#pragma once

#include <so_5/disp/mpsc_queue_traits/h/pub.hpp>
#include <so_5/disp/mpmc_queue_traits/h/pub.hpp>

#include <so_5/h/declspec.hpp>

#include <memory>

namespace so_5 {

//
// queue_locks_defaults_manager_t
//

/*!
 * \brief A base class for manager of default locks for event queues.
 *
 * \since
 * v.5.5.18
 */
class SO_5_TYPE queue_locks_defaults_manager_t
	{
	public :
		queue_locks_defaults_manager_t(
			const queue_locks_defaults_manager_t & ) = delete;
		queue_locks_defaults_manager_t(
			queue_locks_defaults_manager_t && ) = delete;
		queue_locks_defaults_manager_t & operator=(
			const queue_locks_defaults_manager_t & ) = delete;
		queue_locks_defaults_manager_t & operator=(
			queue_locks_defaults_manager_t && ) = delete;

		queue_locks_defaults_manager_t() = default;
		virtual ~queue_locks_defaults_manager_t() SO_5_NOEXCEPT = default;

		//! Get default lock_factory for MPSC queues.
		virtual so_5::disp::mpsc_queue_traits::lock_factory_t
		mpsc_queue_lock_factory() = 0;

		//! Get default lock_factory for MPMC queues.
		virtual so_5::disp::mpmc_queue_traits::lock_factory_t
		mpmc_queue_lock_factory() = 0;
	};

//
// queue_locks_defaults_manager_unique_ptr_t
//

/*!
 * \brief An alias of unique_ptr for queue_locks_defaults_manager.
 *
 * \since
 * v.5.5.18
 */
using queue_locks_defaults_manager_unique_ptr_t =
	std::unique_ptr< queue_locks_defaults_manager_t >;

//
// make_defaults_manager_for_simple_locks
//
/*!
 * \brief A factory for queue_locks_defaults_manager with
 * generators for simple locks.
 *
 * \since
 * v.5.5.18
 */
SO_5_FUNC queue_locks_defaults_manager_unique_ptr_t
make_defaults_manager_for_simple_locks();

//
// make_defaults_manager_for_combined_locks
//
/*!
 * \brief A factory for queue_locks_defaults_manager with
 * generators for combined locks.
 *
 * \since
 * v.5.5.18
 */
SO_5_FUNC queue_locks_defaults_manager_unique_ptr_t
make_defaults_manager_for_combined_locks();

} /* namespace so_5 */


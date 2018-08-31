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

#include <so_5/rt/h/queue_locks_defaults_manager.hpp>

#include <so_5/h/stdcpp.hpp>

namespace so_5 {

namespace {

//
// manager_for_simple_locks_t
//

class manager_for_simple_locks_t
	:	public queue_locks_defaults_manager_t
	{
	public :
		virtual so_5::disp::mpsc_queue_traits::lock_factory_t
		mpsc_queue_lock_factory() override
			{
				return so_5::disp::mpsc_queue_traits::simple_lock_factory();
			}

		virtual so_5::disp::mpmc_queue_traits::lock_factory_t
		mpmc_queue_lock_factory() override
			{
				return so_5::disp::mpmc_queue_traits::simple_lock_factory();
			}
	};

//
// manager_for_combined_locks_t
//

class manager_for_combined_locks_t
	:	public queue_locks_defaults_manager_t
	{
	public :
		virtual so_5::disp::mpsc_queue_traits::lock_factory_t
		mpsc_queue_lock_factory() override
			{
				return so_5::disp::mpsc_queue_traits::combined_lock_factory();
			}

		virtual so_5::disp::mpmc_queue_traits::lock_factory_t
		mpmc_queue_lock_factory() override
			{
				return so_5::disp::mpmc_queue_traits::combined_lock_factory();
			}
	};

} /* namespace anonymous */

//
// make_defaults_manager_for_simple_locks
//
SO_5_FUNC queue_locks_defaults_manager_unique_ptr_t
make_defaults_manager_for_simple_locks()
	{
		return so_5::stdcpp::make_unique< manager_for_simple_locks_t >();
	}

//
// make_defaults_manager_for_combined_locks
//
SO_5_FUNC queue_locks_defaults_manager_unique_ptr_t
make_defaults_manager_for_combined_locks()
	{
		return so_5::stdcpp::make_unique< manager_for_combined_locks_t >();
	}

} /* namespace so_5 */


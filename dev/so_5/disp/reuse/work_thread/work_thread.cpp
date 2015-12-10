/*
	SObjectizer 5.
*/

#include <iostream>
#include <cstdlib>
#include <mutex>
#include <condition_variable>

#include <so_5/rt/h/agent.hpp>
#include <so_5/rt/h/environment.hpp>
#include <so_5/disp/reuse/work_thread/h/work_thread.hpp>

namespace so_5
{

namespace disp
{

namespace reuse
{

namespace work_thread
{

//
// demand_queue
//
demand_queue_t::demand_queue_t(
	queue_traits::lock_unique_ptr_t lock )
	:	m_lock{ std::move(lock) }
	,	m_in_service( false )
{
}

demand_queue_t::~demand_queue_t()
{
	m_demands.clear();
}

void
demand_queue_t::push(
	execution_demand_t demand )
{
	queue_traits::lock_guard_t guard{ *m_lock };

	if( m_in_service )
	{
		const bool demands_empty_before_service = m_demands.empty();

		m_demands.push_back( std::move( demand ) );

		if( demands_empty_before_service )
		{
			// May be someone is waiting...
			// It should be informed about new demands.
			guard.notify_one();
		}
	}
}

int
demand_queue_t::pop(
	demand_container_t & demands,
	demands_counter_t & external_counter )
{
	queue_traits::unique_lock_t lock{ *m_lock };
	while( true )
	{
		if( m_in_service && !m_demands.empty() )
		{
			demands.swap( m_demands );

			// It's time to update external counter.
			external_counter.store( demands.size(), std::memory_order_release );

			break;
		}
		else if( !m_in_service )
			return shutting_down;
		else
		{
			// Queue is empty. We should wait for a demand or
			// a shutdown signal.
			lock.wait_for_notify();
		}
	}

	return demand_extracted;
}

void
demand_queue_t::start_service()
{
	queue_traits::lock_guard_t lock{ *m_lock };

	m_in_service = true;
}

void
demand_queue_t::stop_service()
{
	queue_traits::lock_guard_t lock{ *m_lock };

	m_in_service = false;
	// If the demands queue is empty then someone is waiting
	// for new demands inside pop().
	if( m_demands.empty() )
		lock.notify_one();
}

void
demand_queue_t::clear()
{
	queue_traits::lock_guard_t lock{ *m_lock };

	m_demands.clear();
}

std::size_t
demand_queue_t::demands_count( const demands_counter_t & external_counter )
{
	queue_traits::lock_guard_t lock{ *m_lock };

	return m_demands.size() + external_counter.load( std::memory_order_acquire );
}

//
// work_thread_t
//
work_thread_t::work_thread_t(
	queue_traits::lock_factory_t queue_lock_factory )
	:	m_queue{ queue_lock_factory() }
{
	m_continue_work = WORK_THREAD_STOP;
}

void
work_thread_t::start()
{
	m_queue.start_service();
	m_continue_work = WORK_THREAD_CONTINUE;

	m_thread.reset( new std::thread( [this]() { body(); } ) );
}

void
work_thread_t::shutdown()
{
	m_continue_work = WORK_THREAD_STOP;
	m_queue.stop_service();
}

void
work_thread_t::wait()
{
	m_thread->join();

	m_queue.clear();
}

event_queue_t &
work_thread_t::event_queue()
{
	return m_queue;
}

event_queue_t *
work_thread_t::get_agent_binding()
{
	return &event_queue();
}

std::size_t
work_thread_t::demands_count()
{
	return m_queue.demands_count( m_demands_count );
}

void
work_thread_t::body()
{
	// Store current thread ID to attribute to avoid thread ID
	// request on every event execution.
	m_thread_id = so_5::query_current_thread_id();

	// Local demands queue.
	demand_container_t demands;

	int result = demand_queue_t::no_demands;

	while( m_continue_work == WORK_THREAD_CONTINUE )
	{
		// If the local queue is empty then we should try
		// to get new demands.
		if( demands.empty() )
			result = m_queue.pop( demands, m_demands_count );

		// Serve demands if any.
		if( demand_queue_t::demand_extracted == result )
			serve_demands_block( demands );
	}
}

inline void
work_thread_t::serve_demands_block(
	demand_container_t & demands )
{
	while( !demands.empty() )
	{
		auto & demand = demands.front();

		demand.call_handler( m_thread_id );

		demands.pop_front();
		--m_demands_count;
	}
}

} /* namespace work_thread */

} /* namespace reuse */

} /* namespace disp */

} /* namespace so_5 */


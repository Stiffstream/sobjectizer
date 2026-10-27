/*
 * SObjectizer-5
 */

/*!
 * @file
 * @brief Special event_queue for case when agent should support
 * C++20 coroutines.
 *
 * @since v.5.8.7
 */

#include <so_5/impl/cpp_coro_case_special_queue.hpp>

#include <so_5/agent.hpp>

namespace so_5::impl
{

//
// cpp_coro_case_special_queue_t
//
cpp_coro_case_special_queue_t::demand_unique_ptr_t
cpp_coro_case_special_queue_t::remove_head() noexcept
	{
		demand_unique_ptr_t to_be_deleted{ m_head };
		m_head = m_head->m_next;
		to_be_deleted->m_next = nullptr;

		return to_be_deleted;
	}

void
cpp_coro_case_special_queue_t::store_demand( demand_unique_ptr_t tail_demand )
	{
		if( nullptr == m_head )
			{
				m_head = tail_demand.release();
				m_tail = m_head;
			}
		else
			{
				m_tail->m_next = tail_demand.release();
				m_tail = m_tail->m_next;
			}
	}

cpp_coro_case_special_queue_t::cpp_coro_case_special_queue_t(
	std::mutex & lock )
	: m_lock{ lock }
	{}

cpp_coro_case_special_queue_t::~cpp_coro_case_special_queue_t() noexcept
{
	while( m_head )
		{
			remove_head();
		}
}

void
cpp_coro_case_special_queue_t::bind_actual_queue(
	event_queue_t * actual_queue )
	{
		// NOTE: it's assumed that this method will be called
		// in agent_t::so_bind_to_dispatcher and there is no need
		// to acquire m_lock object.
		m_actual_queue = actual_queue;
	}

void
cpp_coro_case_special_queue_t::push(
	execution_demand_t src_demand )
	{
		agent_t * agent_to_wakeup = src_demand.m_receiver;

		demand_unique_ptr_t wrapped_demand = std::make_unique< demand_t >(
				std::move(src_demand) );

		bool has_to_push_wakeup_demand = false;

		{
			std::lock_guard< std::mutex > lock{ m_lock };

			store_demand( std::move(wrapped_demand) );

			has_to_push_wakeup_demand = m_wakeup_demand_enabled;
			m_wakeup_demand_enabled = false;
		}

		if( has_to_push_wakeup_demand )
//FIXME: should this action be moved to separate helper method?
			// If this attempt fails then it will be impossible
			// to continue the work because the agent won't be
			// informed about the presence of demand to be handled.
			so_5::details::invoke_noexcept_code(
				[this, agent_to_wakeup]() {
					m_actual_queue->push(
							execution_demand_t{
									agent_to_wakeup,
									message_limit::control_block_t::none(),
									null_mbox_id(),
									typeid(msg_wakeup_agent_for_cpp_coro_case),
									message_ref_t{} /* no actual message */,
									agent_t::get_demand_handler_for_cpp_coro_case_ptr()
							} );
				} );
	}

void
cpp_coro_case_special_queue_t::push_evt_start(
	execution_demand_t demand )
	{
		// Simple delegate this call to the actual_queue.
		//
		// We assume that this is the first call to the queue and
		// push() can't be called before push_evt_start.
		m_actual_queue->push_evt_start( std::move(demand) );
	}

void
cpp_coro_case_special_queue_t::push_evt_finish(
	execution_demand_t demand ) noexcept
	{
		bool do_actual_push = false;

		{
			std::lock_guard< std::mutex > lock{ m_lock };

			if( m_wakeup_demand_enabled )
				// The event queue is empty, so we can push evt_finish
				// right now.
				do_actual_push = true;
			else
				{
					// The event queue is not empty or there is the
					// current coroutine. The push of evt_finish has to
					// be postponed.
					m_pending_evt_finish = std::move(demand);
				}
		}

		if( do_actual_push )
			m_actual_queue->push_evt_finish( std::move(demand) );
	}

//FIXME: is there a possibility that wakeup_demand will be pushed to the
//queue just before push_coro_resumption_demand starts its work?
void
cpp_coro_case_special_queue_t::push_coro_resumption_demand(
	execution_demand_t demand )
	{
std::cout << "*** push_coro_resumption_demand called" << std::endl;
		// Delivery of wakeup demands has to be disabled for some time.
		m_wakeup_demand_enabled = false;

		// If this attempt fails then it will be impossible
		// to continue the work because the agent won't be
		// informed about the presence of demand to be handled.
		so_5::details::invoke_noexcept_code(
			[this, &demand]() {
				m_actual_queue->push( std::move(demand) );
			} );

std::cout << "*** coro resumption demand pushed" << std::endl;
	}

cpp_coro_case_special_queue_t::demand_unique_ptr_t
cpp_coro_case_special_queue_t::try_extract_demand_for_execution()
	{
		bool need_push_pending_evt_finish = false;

		{
			std::lock_guard< std::mutex > lock{ m_lock };

			if( m_head )
				return remove_head();

			// The queue is empty now. A case when pending_evt_finish
			// if present must be handled carefully.
			if( m_pending_evt_finish.has_value() )
				{
					// We don't expect ordinary demands after receiving
					// the evt_finish, but it's beter to disable
					// wakeup_demand at all.
					m_wakeup_demand_enabled = false;
					need_push_pending_evt_finish = true;
				}
			else
				// When the next demand arrives the wakeup_demand has
				// to be pushed to the actual_queue.
				m_wakeup_demand_enabled = true;
		}

		if( need_push_pending_evt_finish )
			{
				// We assume that only one thread can do a call
				// to try_extract_demand_for_execution, so there is
				// no need to protect this access to m_pending_evt_finish.
				m_actual_queue->push_evt_finish(
						std::move(*m_pending_evt_finish) );
			}

		return {};
	}

event_queue_t *
cpp_coro_case_special_queue_t::giveaway_actual_queue() noexcept
	{
		event_queue_t * q = m_actual_queue;
		m_actual_queue = nullptr;
		return q;
	}

} /* namespace so_5::impl */


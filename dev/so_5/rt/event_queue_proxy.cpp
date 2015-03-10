/*
	SObjectizer 5.
*/

/*!
	\file
	\since v.5.4.0
	\brief A proxy for event_queue pointer.
*/

#include <so_5/rt/h/event_queue_proxy.hpp>

#include <so_5/rt/h/environment.hpp>

#include <cstdlib>

namespace so_5
{

namespace rt
{

void
event_queue_proxy_t::switch_to_actual_queue(
	event_queue_t & actual_queue,
	agent_t * agent,
	demand_handler_pfn_t start_demand_handler )
	{
		std::lock_guard< std::mutex > lock{ m_lock };

		// All exceptions below would lead to unpredictable
		// application state. Because of that an exception would
		// lead to std::abort().
		try
			{
				m_actual_queue = &actual_queue;

				// The first demand for the agent must be evt_start demand.
				m_actual_queue->push(
						execution_demand_t(
								agent,
								message_limit::control_block_t::none(),
								0,
								typeid(void),
								message_ref_t(),
								start_demand_handler ) );

				if( m_tmp_queue )
					{
						move_tmp_queue_to_actual_queue();
						m_tmp_queue.reset();
					}
			}
		catch( const std::exception & x )
			{
				SO_5_LOG_ERROR( agent->so_environment(), log_stream ) {
					log_stream << "Exception during transferring events from "
							"temporary to the actual event queue. "
							"Work cannot be continued. "
							"Exception: " << x.what();
				}

				std::abort();
			}

		m_status = status_t::started;
	}

event_queue_t *
event_queue_proxy_t::shutdown()
	{
		std::lock_guard< std::mutex > lock{ m_lock };

		auto result = m_actual_queue;

		m_actual_queue = nullptr;
		m_status = status_t::stopped;

		return result;
	}

void
event_queue_proxy_t::push( execution_demand_t demand )
	{
		std::lock_guard< std::mutex > lock{ m_lock };

		if( m_actual_queue )
			m_actual_queue->push( std::move( demand ) );
		else if( status_t::not_started == m_status )
			{
				if( !m_tmp_queue )
					m_tmp_queue.reset( new temporary_queue_t{} );

				m_tmp_queue->push_back( std::move( demand ) );
			}
	}

void
event_queue_proxy_t::move_tmp_queue_to_actual_queue()
	{
		while( !m_tmp_queue->empty() )
			{
				m_actual_queue->push( m_tmp_queue->front() );

				m_tmp_queue->pop_front();
			}
	}

} /* namespace rt */

} /* namespace so_5 */


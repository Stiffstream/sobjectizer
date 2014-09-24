/*
	SObjectizer 5.
*/

/*!
	\file
	\since v.5.4.0
	\brief An implementation of event queue for temporary storing of events.
*/

#include <so_5/rt/h/temporary_event_queue.hpp>

#include <queue>
#include <cstdlib>

#include <so_5/rt/h/environment.hpp>

namespace so_5
{

namespace rt
{

//
// temporary_event_queue_t::temporary_queue_t
//
class temporary_event_queue_t::temporary_queue_t
	{
	public :
		std::queue< execution_demand_t > m_queue;
	};

temporary_event_queue_t::temporary_event_queue_t(
	std::mutex & mutex )
	:	m_mutex( mutex )
	,	m_actual_queue( nullptr )
	{}

temporary_event_queue_t::~temporary_event_queue_t()
	{}

void
temporary_event_queue_t::push( execution_demand_t demand )
	{
		std::lock_guard< std::mutex > lock( m_mutex );

		if( m_actual_queue )
			// Demand must go directly to the actual event queue.
			m_actual_queue->push( std::move( demand ) );
		else
			{
				if( !m_tmp_queue )
					m_tmp_queue.reset( new temporary_queue_t() );

				m_tmp_queue->m_queue.push( std::move( demand ) );
			}
	}

void
temporary_event_queue_t::switch_to_actual_queue(
	event_queue_t & actual_queue,
	agent_t * agent,
	demand_handler_pfn_t start_demand_handler )
	{
		std::lock_guard< std::mutex > lock( m_mutex );

		// All exceptions below would lead to unpredictable
		// application state. Because of that an exception would
		// lead to std::abort().
		try
			{
				actual_queue.push(
						execution_demand_t(
								agent,
								0,
								typeid(void),
								message_ref_t(),
								start_demand_handler ) );

				if( m_tmp_queue )
					while( !m_tmp_queue->m_queue.empty() )
						{
							actual_queue.push( m_tmp_queue->m_queue.front() );

							m_tmp_queue->m_queue.pop();
						}

				m_actual_queue = &actual_queue;
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
	}

} /* namespace rt */

} /* namespace so_5 */


/*
	SObjectizer 5.
*/

#include <algorithm>

#include <cpp_util_2/h/lexcast.hpp>

#include <so_5/rt/impl/coop_dereg/h/coop_dereg_executor_thread.hpp>

namespace so_5
{

namespace rt
{

namespace impl
{

namespace coop_dereg
{

//
// coop_dereg_executor_thread_t
//

coop_dereg_executor_thread_t::coop_dereg_executor_thread_t()
{
}

coop_dereg_executor_thread_t::~coop_dereg_executor_thread_t()
{
}

void
coop_dereg_executor_thread_t::start()
{
	// Queue must be informed.
	m_dereg_demand_queue.start_service();

	m_thread.reset( new std::thread( [this]() { body(); } ) );
}

void
coop_dereg_executor_thread_t::finish()
{
	m_dereg_demand_queue.stop_service();

	m_thread->join();
}

void
coop_dereg_executor_thread_t::push_dereg_demand(
	agent_coop_t * coop )
{
	m_dereg_demand_queue.push( coop );
}

void
exec_final_coop_dereg( agent_coop_t * coop )
{
	agent_coop_t::call_final_deregister_coop( coop );
}

void
coop_dereg_executor_thread_t::body()
{
	dereg_demand_queue_t::dereg_demand_container_t demands;
	do
	{
		demands.clear();
		m_dereg_demand_queue.pop( demands );

		std::for_each(
			demands.begin(),
			demands.end(),
			exec_final_coop_dereg );
	}
	while( !demands.empty() );
}

} /* namespace coop_dereg */

} /* namespace impl */

} /* namespace rt */

} /* namespace so_5 */

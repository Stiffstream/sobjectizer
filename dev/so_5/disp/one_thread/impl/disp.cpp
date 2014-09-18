/*
	SObjectizer 5.
*/

#include <so_5/disp/one_thread/impl/h/disp.hpp>

namespace so_5
{

namespace disp
{

namespace one_thread
{

namespace impl
{

dispatcher_t::dispatcher_t()
	:
		m_work_thread( *self_ptr() )
{
}

dispatcher_t::~dispatcher_t()
{
}

void
dispatcher_t::start()
{
	m_work_thread.start();
}

void
dispatcher_t::shutdown()
{
	m_work_thread.shutdown();
}

void
dispatcher_t::wait()
{
	m_work_thread.wait();
}

so_5::rt::event_queue_t *
dispatcher_t::get_agent_binding()
{
	return m_work_thread.get_agent_binding();
}

} /* namespace impl */

} /* namespace one_thread */

} /* namespace disp */

} /* namespace so_5 */

/*
	SObjectizer 5.
*/

#include <so_5/disp/active_group/impl/h/disp_binder.hpp>
#include <so_5/disp/active_group/impl/h/disp.hpp>

#include <so_5/h/exception.hpp>
#include <so_5/rt/h/agent.hpp>

#include <so_5/disp/reuse/h/disp_binder_helpers.hpp>

namespace so_5
{

namespace disp
{

namespace active_group
{

namespace impl
{

disp_binder_t::disp_binder_t(
	const std::string & disp_name,
	const std::string & group_name )
	:
		m_disp_name( disp_name ),
		m_group_name( group_name )
{
}

disp_binder_t::~disp_binder_t()
{
}

so_5::rt::disp_binding_activator_t
disp_binder_t::bind_agent(
	so_5::rt::so_environment_t & env,
	so_5::rt::agent_ref_t agent_ref )
{
	using so_5::rt::disp_binding_activator_t;
	using namespace so_5::disp::reuse;

	return do_with_dispatcher< disp_binding_activator_t, dispatcher_t >(
		env,
		m_disp_name,
		[this, agent_ref]( dispatcher_t & disp ) -> disp_binding_activator_t
		{
			auto ctx = disp.query_thread_for_group( m_group_name );

			try
			{
				disp_binding_activator_t activator =
					[agent_ref, ctx]() {
						agent_ref->so_bind_to_dispatcher( *ctx );
					};

				return activator;
			}
			catch( ... )
			{
				// Dispatcher for the agent should be removed.
				disp.release_thread_for_group( m_group_name );
				throw;
			}
		} );
}

void
disp_binder_t::unbind_agent(
	so_5::rt::so_environment_t & env,
	so_5::rt::agent_ref_t agent_ref )
{
	using namespace so_5::disp::reuse;

	do_with_dispatcher< void, dispatcher_t >( env, m_disp_name,
		[this]( dispatcher_t & disp )
		{
			disp.release_thread_for_group( m_group_name );
		} );
}

} /* namespace impl */

} /* namespace active_group */

} /* namespace disp */

} /* namespace so_5 */


/*
	SObjectizer 5.
*/

#include <so_5/disp/one_thread/impl/h/disp_binder.hpp>
#include <so_5/disp/one_thread/impl/h/disp.hpp>

#include <so_5/h/exception.hpp>
#include <so_5/rt/h/agent.hpp>

namespace so_5
{

namespace disp
{

namespace one_thread
{

namespace impl
{

disp_binder_t::disp_binder_t(
	const std::string & disp_name )
	:
		m_disp_name( disp_name )
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
	if( m_disp_name.empty() )
		return make_agent_binding(
				&( env.query_default_dispatcher() ),
				std::move( agent_ref ) );
	else
		return make_agent_binding( 
			env.query_named_dispatcher( m_disp_name ).get(),
			std::move( agent_ref ) );
}

void
disp_binder_t::unbind_agent(
	so_5::rt::so_environment_t & env,
	so_5::rt::agent_ref_t agent_ref )
{
}

so_5::rt::disp_binding_activator_t
disp_binder_t::make_agent_binding(
	so_5::rt::dispatcher_t * disp,
	so_5::rt::agent_ref_t agent )
{
	// If the dispatcher is found then the object should be bound to it.
	if( !disp )
		SO_5_THROW_EXCEPTION( rc_named_disp_not_found,
				"dispatcher with name \"" + m_disp_name + "\" not found" );

	// It should be exactly our dispatcher.
	dispatcher_t * d = dynamic_cast< dispatcher_t * >( disp );

	if( nullptr == d )
		SO_5_THROW_EXCEPTION( rc_disp_type_mismatch,
			"disp type mismatch for disp \"" + m_disp_name +
					"\", expected one_thread disp" );

	return [agent, d]() {
		agent->so_bind_to_dispatcher( *(d->get_agent_binding()) );
	};
}

} /* namespace impl */

} /* namespace one_thread */

} /* namespace disp */

} /* namespace so_5 */


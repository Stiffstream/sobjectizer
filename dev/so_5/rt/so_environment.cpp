/*
	SObjectizer 5.
*/

#include <so_5/rt/h/so_environment.hpp>

#include <so_5/rt/impl/h/mbox_core.hpp>
#include <so_5/rt/impl/h/agent_core.hpp>
#include <so_5/rt/impl/h/disp_core.hpp>
#include <so_5/rt/impl/h/layer_core.hpp>

namespace so_5
{

namespace rt
{

//
// so_environment_params_t
//

so_environment_params_t::so_environment_params_t()
	:	m_event_exception_logger( create_std_event_exception_logger() )
	,	m_exception_reaction( abort_on_exception )
	,	m_autoshutdown_disabled( false )
{
}

so_environment_params_t::so_environment_params_t(
	so_environment_params_t && other )
	:	m_named_dispatcher_map( std::move( other.m_named_dispatcher_map ) )
	,	m_timer_thread_factory( std::move( other.m_timer_thread_factory ) )
	,	m_so_layers( std::move( other.m_so_layers ) )
	,	m_coop_listener( std::move( other.m_coop_listener ) )
	,	m_event_exception_logger( std::move( other.m_event_exception_logger ) )
	,	m_exception_reaction( other.m_exception_reaction )
	,	m_autoshutdown_disabled( other.m_autoshutdown_disabled )
{}

so_environment_params_t::~so_environment_params_t()
{
}

so_environment_params_t &
so_environment_params_t::operator=( so_environment_params_t && other )
{
	so_environment_params_t tmp( std::move( other ) );
	this->swap( tmp );

	return *this;
}

void
so_environment_params_t::swap( so_environment_params_t & other )
{
	m_named_dispatcher_map.swap( other.m_named_dispatcher_map );
	m_timer_thread_factory.swap( other.m_timer_thread_factory );
	m_so_layers.swap( other.m_so_layers );
	m_coop_listener.swap( other.m_coop_listener );
	m_event_exception_logger.swap( other.m_event_exception_logger );

	std::swap( m_exception_reaction, other.m_exception_reaction );
	std::swap( m_autoshutdown_disabled, other.m_autoshutdown_disabled );
}

so_environment_params_t &
so_environment_params_t::add_named_dispatcher(
	const nonempty_name_t & name,
	dispatcher_unique_ptr_t dispatcher )
{
	m_named_dispatcher_map[ name.query_name() ] =
		dispatcher_ref_t( dispatcher.release() );
	return *this;
}

so_environment_params_t &
so_environment_params_t::timer_thread(
	so_5::timer_thread_factory_t factory )
{
	m_timer_thread_factory = std::move( factory );
	return  *this;
}

so_environment_params_t &
so_environment_params_t::coop_listener(
	coop_listener_unique_ptr_t coop_listener )
{
	m_coop_listener = std::move( coop_listener );
	return *this;
}

so_environment_params_t &
so_environment_params_t::event_exception_logger(
	event_exception_logger_unique_ptr_t logger )
{
	if( nullptr != logger.get() )
		m_event_exception_logger = std::move( logger );

	return *this;
}

void
so_environment_params_t::add_layer(
	const std::type_index & type,
	so_layer_unique_ptr_t layer_ptr )
{
	m_so_layers[ type ] = so_layer_ref_t( layer_ptr.release() );
}

namespace 
{

/*!
 * \since v.5.5.0
 * \brief Helper function for timer_thread creation.
 */
timer_thread_unique_ptr_t
create_appropriate_timer_thread(
	error_logger_shptr_t error_logger,
	const timer_thread_factory_t & user_factory )
{
	if( user_factory )
		return user_factory( std::move( error_logger ) );
	else
		return create_timer_heap_thread( std::move( error_logger ) );
}

} /* namespace anonymous */

//
// so_environment_t::internals_t
//
/*!
 * \since v.5.5.0
 * \brief Internal details of SObjectizer Environment object.
 */
struct so_environment_t::internals_t
{
	/*!
	 * \since v.5.5.0
	 * \brief Error logger object for this environment.
	 *
	 * \attention Must be the first attribute of the object!
	 * It must be created and initilized first and destroyed last.
	 */
	error_logger_shptr_t m_error_logger;

	//! An utility for mboxes.
	impl::mbox_core_ref_t m_mbox_core;

	//! An utility for agents/cooperations.
	impl::agent_core_t m_agent_core;

	//! An utility for dispatchers.
	impl::disp_core_t m_disp_core;

	//! An utility for layers.
	impl::layer_core_t m_layer_core;

	//! Timer.
	so_5::timer_thread_unique_ptr_t m_timer_thread;

	/*!
	 * \since v.5.3.0
	 * \brief An exception reaction for the whole SO Environment.
	 */
	const exception_reaction_t m_exception_reaction;

	/*!
	 * \since v.5.4.0
	 * \brief Is autoshutdown when there is no more cooperation disabled?
	 *
	 * \see so_environment_params_t::disable_autoshutdown()
	 */
	const bool m_autoshutdown_disabled;

	//! Constructor.
	internals_t(
		so_environment_t & env,
		so_environment_params_t && params )
//FIXME: error_logger object must be taken from so_environment_params!
		:	m_error_logger( create_stderr_logger() )
		,	m_mbox_core( new impl::mbox_core_t() )
		,	m_agent_core(
				env,
				params.so5__giveout_coop_listener() )
		,	m_disp_core(
				env,
				params.so5__named_dispatcher_map(),
				params.so5__giveout_event_exception_logger() )
		,	m_layer_core(
				env,
				params.so5__layers_map() )
		,	m_timer_thread(
				create_appropriate_timer_thread(
						m_error_logger,
						params.so5__giveout_timer_thread_factory() ) )
		,	m_exception_reaction( params.exception_reaction() )
		,	m_autoshutdown_disabled( params.autoshutdown_disabled() )
	{}
};

//
// so_environment_t
//

so_environment_t &
so_environment_t::self_ref()
{
	return *this;
}


so_environment_t::so_environment_t(
	so_environment_params_t && params )
	:	m_impl( new internals_t( self_ref(), std::move(params) ) )
{
}

so_environment_t::~so_environment_t()
{
}

mbox_ref_t
so_environment_t::create_local_mbox( )
{
	return m_impl->m_mbox_core->create_local_mbox();
}

mbox_ref_t
so_environment_t::create_local_mbox(
	const nonempty_name_t & nonempty_name )
{
	return m_impl->m_mbox_core->create_local_mbox( nonempty_name );
}

dispatcher_t &
so_environment_t::query_default_dispatcher()
{
	return m_impl->m_disp_core.query_default_dispatcher();
}

dispatcher_ref_t
so_environment_t::query_named_dispatcher(
	const std::string & disp_name )
{
	return m_impl->m_disp_core.query_named_dispatcher( disp_name );
}

dispatcher_ref_t
so_environment_t::add_dispatcher_if_not_exists(
	const std::string & disp_name,
	std::function< dispatcher_unique_ptr_t() > disp_factory )
{
	return m_impl->m_disp_core.add_dispatcher_if_not_exists(
			disp_name,
			disp_factory );
}

void
so_environment_t::install_exception_logger(
	event_exception_logger_unique_ptr_t logger )
{
	m_impl->m_disp_core.install_exception_logger( std::move( logger ) );
}

agent_coop_unique_ptr_t
so_environment_t::create_coop(
	const nonempty_name_t & name )
{
	return create_coop(
		name,
		create_default_disp_binder() );
}

agent_coop_unique_ptr_t
so_environment_t::create_coop(
	const nonempty_name_t & name,
	disp_binder_unique_ptr_t disp_binder )
{
	return agent_coop_unique_ptr_t(
			new agent_coop_t( name, std::move(disp_binder), self_ref() ) );
}

void
so_environment_t::register_coop(
	agent_coop_unique_ptr_t agent_coop )
{
	m_impl->m_agent_core.register_coop( std::move( agent_coop ) );
}

void
so_environment_t::deregister_coop(
	const nonempty_name_t & name,
	int reason )
{
	m_impl->m_agent_core.deregister_coop( name,
			coop_dereg_reason_t( reason ) );
}

so_5::timer_id_t
so_environment_t::schedule_timer(
	const std::type_index & type_wrapper,
	const message_ref_t & msg,
	const mbox_ref_t & mbox,
	unsigned int delay_msec,
	unsigned int period_msec )
{
	return m_impl->m_timer_thread->schedule(
			type_wrapper,
			mbox,
			msg,
			std::chrono::milliseconds( delay_msec ),
			std::chrono::milliseconds( period_msec ) );
}

void
so_environment_t::single_timer(
	const std::type_index & type_wrapper,
	const message_ref_t & msg,
	const mbox_ref_t & mbox,
	unsigned int delay_msec )
{
	return m_impl->m_timer_thread->schedule_anonymous(
			type_wrapper,
			mbox,
			msg,
			std::chrono::milliseconds( delay_msec ),
			std::chrono::milliseconds::zero() );
}

so_layer_t *
so_environment_t::query_layer(
	const std::type_index & type ) const
{
	return m_impl->m_layer_core.query_layer( type );
}

void
so_environment_t::add_extra_layer(
	const std::type_index & type,
	const so_layer_ref_t & layer )
{
	m_impl->m_layer_core.add_extra_layer( type, layer );
}

void
so_environment_t::run()
{
//FIXME: implement this!
}

void
so_environment_t::stop()
{
//FIXME: implement this!
}

void
so_environment_t::call_exception_logger(
	const std::exception & event_exception,
	const std::string & coop_name )
{
	m_impl->m_disp_core.call_exception_logger( event_exception, coop_name );
}

exception_reaction_t
so_environment_t::exception_reaction() const
{
	return m_impl->m_exception_reaction;
}

error_logger_t &
so_environment_t::error_logger() const
{
	return *(m_impl->m_error_logger);
}

mbox_ref_t
so_environment_t::so5__create_mpsc_mbox(
	agent_t * single_consumer,
	event_queue_proxy_ref_t event_queue )
{
	return m_impl->m_mbox_core->create_mpsc_mbox(
			single_consumer,
			std::move( event_queue ) );
}

void
so_environment_t::so5__ready_to_deregister_notify(
	agent_coop_t * coop )
{
	m_impl->m_agent_core.ready_to_deregister_notify( coop );
}

void
so_environment_t::so5__final_deregister_coop(
	const std::string & coop_name )
{
	bool any_cooperation_alive = 
			m_impl->m_agent_core.final_deregister_coop( coop_name );

	if( !any_cooperation_alive && !m_impl->m_autoshutdown_disabled )
		stop();
}

} /* namespace rt */

} /* namespace so_5 */


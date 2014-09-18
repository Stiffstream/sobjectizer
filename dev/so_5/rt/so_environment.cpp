/*
	SObjectizer 5.
*/

#include <so_5/rt/impl/h/so_environment_impl.hpp>

#include <so_5/rt/h/so_environment.hpp>

namespace so_5
{

namespace rt
{

//
// so_environment_params_t
//

so_environment_params_t::so_environment_params_t()
	:	m_mbox_mutex_pool_size( 128 )
	,	m_agent_coop_mutex_pool_size( 32 )
	,	m_agent_event_queue_mutex_pool_size( 128 )
	,	m_event_exception_logger( create_std_event_exception_logger() )
	,	m_exception_reaction( abort_on_exception )
	,	m_autoshutdown_disabled( false )
{
}

so_environment_params_t::so_environment_params_t(
	so_environment_params_t && other )
	:	m_mbox_mutex_pool_size( other.m_mbox_mutex_pool_size )
	,	m_agent_coop_mutex_pool_size( other.m_agent_coop_mutex_pool_size )
	,	m_agent_event_queue_mutex_pool_size(
			other.m_agent_event_queue_mutex_pool_size )
	,	m_named_dispatcher_map( std::move( other.m_named_dispatcher_map ) )
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
	std::swap( m_mbox_mutex_pool_size, other.m_mbox_mutex_pool_size );
	std::swap( m_agent_coop_mutex_pool_size,
			other.m_agent_coop_mutex_pool_size );
	std::swap( m_agent_event_queue_mutex_pool_size,
			other.m_agent_event_queue_mutex_pool_size );
	m_named_dispatcher_map.swap( other.m_named_dispatcher_map );
	m_timer_thread_factory.swap( other.m_timer_thread_factory );
	m_so_layers.swap( other.m_so_layers );
	m_coop_listener.swap( other.m_coop_listener );
	m_event_exception_logger.swap( other.m_event_exception_logger );

	std::swap( m_exception_reaction, other.m_exception_reaction );
	std::swap( m_autoshutdown_disabled, other.m_autoshutdown_disabled );
}

so_environment_params_t &
so_environment_params_t::mbox_mutex_pool_size(
	unsigned int mutex_pool_size )
{
	m_mbox_mutex_pool_size = mutex_pool_size;
	return * this;
}

so_environment_params_t &
so_environment_params_t::agent_coop_mutex_pool_size(
	unsigned int mutex_pool_size )
{
	m_agent_coop_mutex_pool_size = mutex_pool_size;
	return * this;
}

so_environment_params_t &
so_environment_params_t::agent_event_queue_mutex_pool_size(
	unsigned int mutex_pool_size )
{
	m_agent_event_queue_mutex_pool_size = mutex_pool_size;
	return * this;
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

unsigned int
so_environment_params_t::mbox_mutex_pool_size() const
{
	return m_mbox_mutex_pool_size;
}

unsigned int
so_environment_params_t::agent_coop_mutex_pool_size() const
{
	return m_agent_coop_mutex_pool_size;
}

unsigned int
so_environment_params_t::agent_event_queue_mutex_pool_size() const
{
	return m_agent_event_queue_mutex_pool_size;
}

const named_dispatcher_map_t &
so_environment_params_t::named_dispatcher_map() const
{
	return m_named_dispatcher_map;
}

const so_layer_map_t &
so_environment_params_t::so_layers_map() const
{
	return m_so_layers;
}

//
// so_environment_t
//

so_environment_t &
so_environment_t::self_ref()
{
	return *this;
}


so_environment_t::so_environment_t(
	so_environment_params_t && so_environment_params )
	:
		m_so_environment_impl(
			new impl::so_environment_impl_t(
				std::move(so_environment_params),
				self_ref() ) )
{
}

so_environment_t::~so_environment_t()
{
	if( m_so_environment_impl )
		delete m_so_environment_impl;
}

mbox_ref_t
so_environment_t::create_local_mbox( )
{
	return m_so_environment_impl->create_local_mbox();
}

mbox_ref_t
so_environment_t::create_local_mbox(
	const nonempty_name_t & nonempty_name )
{
	return m_so_environment_impl->create_local_mbox( nonempty_name );
}

dispatcher_t &
so_environment_t::query_default_dispatcher()
{
	return m_so_environment_impl->query_default_dispatcher();
}

dispatcher_ref_t
so_environment_t::query_named_dispatcher(
	const std::string & disp_name )
{
	return m_so_environment_impl
		->query_named_dispatcher( disp_name );
}

dispatcher_ref_t
so_environment_t::add_dispatcher_if_not_exists(
	const std::string & disp_name,
	std::function< dispatcher_unique_ptr_t() > disp_factory )
{
	return m_so_environment_impl->add_dispatcher_if_not_exists(
			disp_name,
			disp_factory );
}

void
so_environment_t::install_exception_logger(
	event_exception_logger_unique_ptr_t logger )
{
	m_so_environment_impl->install_exception_logger( std::move( logger ) );
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
	m_so_environment_impl->register_coop( std::move( agent_coop ) );
}

void
so_environment_t::deregister_coop(
	const nonempty_name_t & name,
	int reason )
{
	m_so_environment_impl->deregister_coop( name, reason );
}

so_5::timer_id_t
so_environment_t::schedule_timer(
	const std::type_index & type_wrapper,
	const message_ref_t & msg,
	const mbox_ref_t & mbox,
	unsigned int delay_msec,
	unsigned int period_msec )
{
	return m_so_environment_impl->schedule_timer(
		type_wrapper, msg, mbox, delay_msec, period_msec );
}

void
so_environment_t::single_timer(
	const std::type_index & type_wrapper,
	const message_ref_t & msg,
	const mbox_ref_t & mbox,
	unsigned int delay_msec )
{
	m_so_environment_impl->single_timer(
		type_wrapper, msg, mbox, delay_msec );
}

so_layer_t *
so_environment_t::query_layer(
	const std::type_index & type ) const
{
	return m_so_environment_impl->query_layer( type );
}

void
so_environment_t::add_extra_layer(
	const std::type_index & type,
	const so_layer_ref_t & layer )
{
	m_so_environment_impl->add_extra_layer( type, layer );
}

void
so_environment_t::run()
{
	m_so_environment_impl->run( *this );
}

void
so_environment_t::stop()
{
	m_so_environment_impl->stop();
}

impl::so_environment_impl_t &
so_environment_t::so_environment_impl()
{
	return *m_so_environment_impl;
}

void
so_environment_t::call_exception_logger(
	const std::exception & event_exception,
	const std::string & coop_name )
{
	m_so_environment_impl->call_exception_logger( event_exception, coop_name );
}

exception_reaction_t
so_environment_t::exception_reaction() const
{
	return m_so_environment_impl->exception_reaction();
}

error_logger_t &
so_environment_t::error_logger() const
{
	return m_so_environment_impl->error_logger();
}

} /* namespace rt */

} /* namespace so_5 */


/*
	SObjectizer 5.
*/

#include <so_5/rt/h/environment.hpp>

#include <string>

#include <so_5/rt/impl/h/internal_env_iface.hpp>

#include <so_5/rt/impl/h/mbox_core.hpp>
#include <so_5/rt/impl/h/agent_core.hpp>
#include <so_5/rt/impl/h/disp_repository.hpp>
#include <so_5/rt/impl/h/layer_core.hpp>

#include <so_5/rt/stats/impl/h/std_controller.hpp>
#include <so_5/rt/stats/impl/h/ds_mbox_core_stats.hpp>
#include <so_5/rt/stats/impl/h/ds_agent_core_stats.hpp>
#include <so_5/rt/stats/impl/h/ds_timer_thread_stats.hpp>

#include <so_5/details/h/rollback_on_exception.hpp>

namespace so_5
{

//
// environment_params_t
//

environment_params_t::environment_params_t()
	:	m_event_exception_logger( create_std_event_exception_logger() )
	,	m_exception_reaction( abort_on_exception )
	,	m_autoshutdown_disabled( false )
	,	m_error_logger( create_stderr_logger() )
{
}

environment_params_t::environment_params_t(
	environment_params_t && other )
	:	m_named_dispatcher_map( std::move( other.m_named_dispatcher_map ) )
	,	m_timer_thread_factory( std::move( other.m_timer_thread_factory ) )
	,	m_so_layers( std::move( other.m_so_layers ) )
	,	m_coop_listener( std::move( other.m_coop_listener ) )
	,	m_event_exception_logger( std::move( other.m_event_exception_logger ) )
	,	m_exception_reaction( other.m_exception_reaction )
	,	m_autoshutdown_disabled( other.m_autoshutdown_disabled )
	,	m_error_logger( std::move( other.m_error_logger ) )
	,	m_message_delivery_tracer( std::move( other.m_message_delivery_tracer ) )
{}

environment_params_t::~environment_params_t()
{
}

environment_params_t &
environment_params_t::operator=( environment_params_t && other )
{
	environment_params_t tmp( std::move( other ) );
	this->swap( tmp );

	return *this;
}

void
environment_params_t::swap( environment_params_t & other )
{
	m_named_dispatcher_map.swap( other.m_named_dispatcher_map );
	m_timer_thread_factory.swap( other.m_timer_thread_factory );
	m_so_layers.swap( other.m_so_layers );
	m_coop_listener.swap( other.m_coop_listener );
	m_event_exception_logger.swap( other.m_event_exception_logger );

	std::swap( m_exception_reaction, other.m_exception_reaction );
	std::swap( m_autoshutdown_disabled, other.m_autoshutdown_disabled );

	m_error_logger.swap( other.m_error_logger );
	m_message_delivery_tracer.swap( other.m_message_delivery_tracer );
}

environment_params_t &
environment_params_t::add_named_dispatcher(
	const nonempty_name_t & name,
	dispatcher_unique_ptr_t dispatcher )
{
	m_named_dispatcher_map[ name.query_name() ] =
		dispatcher_ref_t( dispatcher.release() );
	return *this;
}

environment_params_t &
environment_params_t::timer_thread(
	so_5::timer_thread_factory_t factory )
{
	m_timer_thread_factory = std::move( factory );
	return  *this;
}

environment_params_t &
environment_params_t::coop_listener(
	coop_listener_unique_ptr_t coop_listener )
{
	m_coop_listener = std::move( coop_listener );
	return *this;
}

environment_params_t &
environment_params_t::event_exception_logger(
	event_exception_logger_unique_ptr_t logger )
{
	if( nullptr != logger.get() )
		m_event_exception_logger = std::move( logger );

	return *this;
}

void
environment_params_t::add_layer(
	const std::type_index & type,
	layer_unique_ptr_t layer_ptr )
{
	m_so_layers[ type ] = layer_ref_t( layer_ptr.release() );
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

/*!
 * \since v.5.5.4
 * \brief A bunch of data sources for core objects.
 */
class core_data_sources_t
	{
	public :
		core_data_sources_t(
			stats::repository_t & ds_repository,
			impl::mbox_core_t & mbox_repository,
			impl::agent_core_t & coop_repository,
			so_5::timer_thread_t & timer_thread )
			:	m_mbox_repository( ds_repository, mbox_repository )
			,	m_coop_repository( ds_repository, coop_repository )
			,	m_timer_thread( ds_repository, timer_thread )
			{}

	private :
		//! Data source for mboxes repository.
		stats::impl::ds_mbox_core_stats_t m_mbox_repository;
		//! Data source for cooperations repository.
		stats::impl::ds_agent_core_stats_t m_coop_repository;
		//! Data source for timer thread.
		stats::impl::ds_timer_thread_stats_t m_timer_thread;
	};

} /* namespace anonymous */

//
// environment_t::internals_t
//
/*!
 * \since v.5.5.0
 * \brief Internal details of SObjectizer Environment object.
 */
struct environment_t::internals_t
{
	/*!
	 * \since v.5.5.0
	 * \brief Error logger object for this environment.
	 *
	 * \attention Must be the first attribute of the object!
	 * It must be created and initilized first and destroyed last.
	 */
	error_logger_shptr_t m_error_logger;

	/*!
	 * \since v.5.5.9
	 * \brief Tracer object for message delivery tracing.
	 * \attention This field must be declared and initialized
	 * before m_mbox_core because a pointer to tracer will be passed
	 * to the constructor of m_mbox_core.
	 */
	so_5::msg_tracing::tracer_unique_ptr_t m_message_delivery_tracer;

	//! An utility for mboxes.
	impl::mbox_core_ref_t m_mbox_core;

	//! An utility for agents/cooperations.
	impl::agent_core_t m_agent_core;

	//! A repository of dispatchers.
	impl::disp_repository_t m_dispatchers;

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
	 * \see environment_params_t::disable_autoshutdown()
	 */
	const bool m_autoshutdown_disabled;

	/*!
	 * \since v.5.5.1
	 * \brief A counter for automatically generated cooperation names.
	 */
	std::atomic_uint_fast64_t m_autoname_counter = { 0 }; 

	/*!
	 * \since v.5.5.4
	 * \brief A controller for run-time monitoring.
	 */
	stats::impl::std_controller_t m_stats_controller;

	/*!
	 * \since v.5.5.4
	 * \brief Data sources for core objects.
	 *
	 * \attention This instance must be created after m_stats_controller
	 * and destroyed before it. Because of that m_core_data_sources declared
	 * after m_stats_controller and after all corresponding objects.
	 */
	core_data_sources_t m_core_data_sources;

	//! Constructor.
	internals_t(
		environment_t & env,
		environment_params_t && params )
		:	m_error_logger( params.so5__error_logger() )
		,	m_message_delivery_tracer{
				params.so5__giveout_message_delivery_tracer() }
		,	m_mbox_core(
				new impl::mbox_core_t{ m_message_delivery_tracer.get() } )
		,	m_agent_core(
				env,
				params.so5__giveout_coop_listener() )
		,	m_dispatchers(
				env,
				params.so5__giveout_named_dispatcher_map(),
				params.so5__giveout_event_exception_logger(),
				params.default_disp_params() )
		,	m_layer_core(
				env,
				params.so5__layers_map() )
		,	m_timer_thread(
				create_appropriate_timer_thread(
						m_error_logger,
						params.so5__giveout_timer_thread_factory() ) )
		,	m_exception_reaction( params.exception_reaction() )
		,	m_autoshutdown_disabled( params.autoshutdown_disabled() )
		,	m_stats_controller(
				// A special mbox for distributing monitoring information
				// must be created and passed to stats_controller.
				m_mbox_core->create_mbox() )
		,	m_core_data_sources(
				m_stats_controller,
				*m_mbox_core,
				m_agent_core,
				*m_timer_thread )
	{}
};

//
// environment_t
//

environment_t &
environment_t::self_ref()
{
	return *this;
}


environment_t::environment_t(
	environment_params_t && params )
	:	m_impl( new internals_t( self_ref(), std::move(params) ) )
{
}

environment_t::~environment_t()
{
}

mbox_t
environment_t::create_mbox( )
{
	return m_impl->m_mbox_core->create_mbox();
}

mbox_t
environment_t::create_mbox(
	const nonempty_name_t & nonempty_name )
{
	return m_impl->m_mbox_core->create_mbox( nonempty_name );
}

mchain_t
environment_t::create_mchain(
	const mchain_params_t & params )
{
	return m_impl->m_mbox_core->create_mchain( *this, params );
}

dispatcher_t &
environment_t::query_default_dispatcher()
{
	return m_impl->m_dispatchers.query_default_dispatcher();
}

dispatcher_ref_t
environment_t::query_named_dispatcher(
	const std::string & disp_name )
{
	return m_impl->m_dispatchers.query_named_dispatcher( disp_name );
}

dispatcher_ref_t
environment_t::add_dispatcher_if_not_exists(
	const std::string & disp_name,
	std::function< dispatcher_unique_ptr_t() > disp_factory )
{
	return m_impl->m_dispatchers.add_dispatcher_if_not_exists(
			disp_name,
			disp_factory );
}

void
environment_t::install_exception_logger(
	event_exception_logger_unique_ptr_t logger )
{
	m_impl->m_dispatchers.install_exception_logger( std::move( logger ) );
}

coop_unique_ptr_t
environment_t::create_coop(
	const nonempty_name_t & name )
{
	return create_coop(
		name,
		create_default_disp_binder() );
}

coop_unique_ptr_t
environment_t::create_coop(
	autoname_indicator_t indicator() )
{
	return create_coop( indicator, create_default_disp_binder() );
}

coop_unique_ptr_t
environment_t::create_coop(
	const nonempty_name_t & name,
	disp_binder_unique_ptr_t disp_binder )
{
	return coop_unique_ptr_t(
			new coop_t( name, std::move(disp_binder), self_ref() ) );
}

coop_unique_ptr_t
environment_t::create_coop(
	autoname_indicator_t (*)(),
	disp_binder_unique_ptr_t disp_binder )
{
	auto counter = ++(m_impl->m_autoname_counter);
	std::string name = "__so5_autoname_" + std::to_string(counter) + "__";
	return coop_unique_ptr_t(
			new coop_t(
					std::move(name),
					std::move(disp_binder),
					self_ref() ) );
}

void
environment_t::register_coop(
	coop_unique_ptr_t agent_coop )
{
	m_impl->m_agent_core.register_coop( std::move( agent_coop ) );
}

void
environment_t::deregister_coop(
	const nonempty_name_t & name,
	int reason )
{
	m_impl->m_agent_core.deregister_coop( name,
			coop_dereg_reason_t( reason ) );
}

so_5::timer_id_t
environment_t::schedule_timer(
	const std::type_index & type_wrapper,
	const message_ref_t & msg,
	const mbox_t & mbox,
	std::chrono::steady_clock::duration pause,
	std::chrono::steady_clock::duration period )
{
	return m_impl->m_timer_thread->schedule(
			type_wrapper,
			mbox,
			msg,
			pause,
			period );
}

void
environment_t::single_timer(
	const std::type_index & type_wrapper,
	const message_ref_t & msg,
	const mbox_t & mbox,
	std::chrono::steady_clock::duration pause )
{
	m_impl->m_timer_thread->schedule_anonymous(
			type_wrapper,
			mbox,
			msg,
			pause,
			std::chrono::milliseconds::zero() );
}

layer_t *
environment_t::query_layer(
	const std::type_index & type ) const
{
	return m_impl->m_layer_core.query_layer( type );
}

void
environment_t::add_extra_layer(
	const std::type_index & type,
	const layer_ref_t & layer )
{
	m_impl->m_layer_core.add_extra_layer( type, layer );
}

void
environment_t::run()
{
	try
	{
		impl__run_stats_controller_and_go_further();
	}
	catch( const so_5::exception_t & )
	{
		// Rethrow our exception because it already has all information.
		throw;
	}
	catch( const std::exception & x )
	{
		SO_5_THROW_EXCEPTION(
				rc_environment_error,
				std::string( "some unexpected error during "
						"environment launching: " ) + x.what() );
	}
}

void
environment_t::stop()
{
	// Sends shutdown signal for all agents.
	m_impl->m_agent_core.start_deregistration();
}

void
environment_t::call_exception_logger(
	const std::exception & event_exception,
	const std::string & coop_name )
{
	m_impl->m_dispatchers.call_exception_logger( event_exception, coop_name );
}

exception_reaction_t
environment_t::exception_reaction() const
{
	return m_impl->m_exception_reaction;
}

error_logger_t &
environment_t::error_logger() const
{
	return *(m_impl->m_error_logger);
}

stats::controller_t &
environment_t::stats_controller()
{
	return m_impl->m_stats_controller;
}

stats::repository_t &
environment_t::stats_repository()
{
	return m_impl->m_stats_controller;
}

void
environment_t::impl__run_stats_controller_and_go_further()
{
	impl__do_run_stage(
			"run_stats_controller",
			[this] {
				/* there is no need to turn_on controller automatically */
			},
			[this] { m_impl->m_stats_controller.turn_off(); },
			[this] { impl__run_layers_and_go_further(); } );
}

void
environment_t::impl__run_layers_and_go_further()
{
	impl__do_run_stage(
			"run_layers",
			[this] { m_impl->m_layer_core.start(); },
			[this] { m_impl->m_layer_core.finish(); },
			[this] { impl__run_dispatcher_and_go_further(); } );
}

void
environment_t::impl__run_dispatcher_and_go_further()
{
	impl__do_run_stage(
			"run_dispatcher",
			[this] { m_impl->m_dispatchers.start(); },
			[this] { m_impl->m_dispatchers.finish(); },
			[this] { impl__run_timer_and_go_further(); } );
}

void
environment_t::impl__run_timer_and_go_further()
{
	impl__do_run_stage(
			"run_timer",
			[this] { m_impl->m_timer_thread->start(); },
			[this] { m_impl->m_timer_thread->finish(); },
			[this] { impl__run_agent_core_and_go_further(); } );
}

void
environment_t::impl__run_agent_core_and_go_further()
{
	impl__do_run_stage(
			"run_agent_core",
			[this] { m_impl->m_agent_core.start(); },
			[this] { m_impl->m_agent_core.finish(); },
			[this] { impl__run_user_supplied_init_and_wait_for_stop(); } );
}

namespace autoshutdown_guard
{
	//! An empty agent for the special cooperation for protection of
	//! init function from autoshutdown feature.
	class a_empty_agent_t : public agent_t
	{
		public :
			a_empty_agent_t( environment_t & env )
				:	agent_t( env )
			{}
	};

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-prototypes"
#endif

	void
	register_init_guard_cooperation(
		environment_t & env,
		bool autoshutdown_disabled )
	{
		if( !autoshutdown_disabled )
			env.register_agent_as_coop(
					"__so_5__init_autoshutdown_guard__",
					new a_empty_agent_t( env ) );
	}

	void
	deregistr_init_guard_cooperation(
		environment_t & env,
		bool autoshutdown_disabled )
	{
		if( !autoshutdown_disabled )
			env.deregister_coop(
					"__so_5__init_autoshutdown_guard__",
					dereg_reason::normal );
	}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

}

void
environment_t::impl__run_user_supplied_init_and_wait_for_stop()
{
	so_5::details::do_with_rollback_on_exception(
		[this]
		{
			// init method must be protected from autoshutdown feature.
			autoshutdown_guard::register_init_guard_cooperation(
					*this,
					m_impl->m_autoshutdown_disabled );

			// Initilizing environment.
			init();

			// Protection is no more needed.
			autoshutdown_guard::deregistr_init_guard_cooperation(
					*this,
					m_impl->m_autoshutdown_disabled );

			m_impl->m_agent_core.wait_for_start_deregistration();
		},
		[this]
		{
			stop();
			m_impl->m_agent_core.wait_for_start_deregistration();
		} );
}

void
environment_t::impl__do_run_stage(
	const std::string & stage_name,
	std::function< void() > init_fn,
	std::function< void() > deinit_fn,
	std::function< void() > next_stage )
{
	try
	{
		init_fn();
	}
	catch( const std::exception & x )
	{
		SO_5_THROW_EXCEPTION(
				rc_unexpected_error,
				stage_name + ": initialization failed, exception is: '" +
				x.what() + "'" );
	}

	try
	{
		next_stage();
	}
	catch( const std::exception & x )
	{
		try
		{
			deinit_fn();
		}
		catch( const std::exception & nested )
		{
			SO_5_THROW_EXCEPTION(
					rc_unexpected_error,
					stage_name + ": deinitialization failed during "
					"exception handling. Original exception is: '" + x.what() +
					"', deinitialization exception is: '" + nested.what() + "'" );
		}

		throw;
	}

	try
	{
		deinit_fn();
	}
	catch( const std::exception & x )
	{
		SO_5_THROW_EXCEPTION(
				rc_unexpected_error,
				stage_name + ": deinitialization failed, exception is: '" +
				x.what() + "'" );
	}
}

namespace impl
{

mbox_t
internal_env_iface_t::create_mpsc_mbox(
	agent_t * single_consumer,
	const message_limit::impl::info_storage_t * limits_storage )
{
	return m_env.m_impl->m_mbox_core->create_mpsc_mbox(
			single_consumer,
			limits_storage );
}

void
internal_env_iface_t::ready_to_deregister_notify(
	coop_t * coop )
{
	m_env.m_impl->m_agent_core.ready_to_deregister_notify( coop );
}

void
internal_env_iface_t::final_deregister_coop(
	const std::string & coop_name )
{
	bool any_cooperation_alive = 
			m_env.m_impl->m_agent_core.final_deregister_coop( coop_name );

	if( !any_cooperation_alive && !m_env.m_impl->m_autoshutdown_disabled )
		m_env.stop();
}

bool
internal_env_iface_t::is_msg_tracing_enabled() const
{
	return nullptr != m_env.m_impl->m_message_delivery_tracer.get();
}

so_5::msg_tracing::tracer_t &
internal_env_iface_t::msg_tracer() const
{
	if( !is_msg_tracing_enabled() )
		SO_5_THROW_EXCEPTION( rc_msg_tracing_disabled,
				"msg_tracer cannot be accessed because msg_tracing is disabled" );

	return *(m_env.m_impl->m_message_delivery_tracer);
}

} /* namespace impl */

} /* namespace so_5 */


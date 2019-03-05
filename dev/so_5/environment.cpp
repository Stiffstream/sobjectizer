/*
	SObjectizer 5.
*/

#include <so_5/environment.hpp>

#include <string>

#include <so_5/impl/internal_env_iface.hpp>

#include <so_5/impl/mbox_core.hpp>
#include <so_5/impl/layer_core.hpp>
#include <so_5/impl/stop_guard_repo.hpp>
#include <so_5/impl/std_msg_tracer_holder.hpp>

#include <so_5/impl/run_stage.hpp>

#include <so_5/stats/impl/std_controller.hpp>
#include <so_5/stats/impl/ds_mbox_core_stats.hpp>
#include <so_5/stats/impl/ds_agent_core_stats.hpp>
#include <so_5/stats/impl/ds_timer_thread_stats.hpp>

#include <so_5/env_infrastructures.hpp>

#include <so_5/details/rollback_on_exception.hpp>

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
	,	m_work_thread_activity_tracking(
			work_thread_activity_tracking_t::unspecified )
	,	m_infrastructure_factory( env_infrastructures::default_mt::factory() )
	,	m_event_queue_hook( make_empty_event_queue_hook_unique_ptr() )
{
}

environment_params_t::environment_params_t(
	environment_params_t && other )
	:	m_timer_thread_factory( std::move( other.m_timer_thread_factory ) )
	,	m_so_layers( std::move( other.m_so_layers ) )
	,	m_coop_listener( std::move( other.m_coop_listener ) )
	,	m_event_exception_logger( std::move( other.m_event_exception_logger ) )
	,	m_exception_reaction( other.m_exception_reaction )
	,	m_autoshutdown_disabled( other.m_autoshutdown_disabled )
	,	m_error_logger( std::move( other.m_error_logger ) )
	,	m_message_delivery_tracer( std::move( other.m_message_delivery_tracer ) )
	,	m_message_delivery_tracer_filter(
			std::move( other.m_message_delivery_tracer_filter ) )
	,	m_work_thread_activity_tracking(
			work_thread_activity_tracking_t::unspecified )
	,	m_queue_locks_defaults_manager( std::move( other.m_queue_locks_defaults_manager ) )
	,	m_infrastructure_factory( std::move(other.m_infrastructure_factory) )
	,	m_event_queue_hook( std::move(other.m_event_queue_hook) )
{}

environment_params_t::~environment_params_t()
{
}

environment_params_t &
environment_params_t::operator=( environment_params_t && other )
{
	environment_params_t tmp( std::move( other ) );
	swap( *this, tmp );

	return *this;
}

SO_5_FUNC void
swap( environment_params_t & a, environment_params_t & b )
{
	using std::swap;

	swap( a.m_timer_thread_factory, b.m_timer_thread_factory );
	swap( a.m_so_layers, b.m_so_layers );
	swap( a.m_coop_listener, b.m_coop_listener );
	swap( a.m_event_exception_logger, b.m_event_exception_logger );

	swap( a.m_exception_reaction, b.m_exception_reaction );
	swap( a.m_autoshutdown_disabled, b.m_autoshutdown_disabled );

	swap( a.m_error_logger, b.m_error_logger );
	swap( a.m_message_delivery_tracer, b.m_message_delivery_tracer );
	swap( a.m_message_delivery_tracer_filter, b.m_message_delivery_tracer_filter );

	swap( a.m_work_thread_activity_tracking, b.m_work_thread_activity_tracking );

	swap( a.m_queue_locks_defaults_manager, b.m_queue_locks_defaults_manager );

	swap( a.m_infrastructure_factory, b.m_infrastructure_factory );

	swap( a.m_event_queue_hook, b.m_event_queue_hook );
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
 * \brief A bunch of data sources for core objects.
 * \since
 * v.5.5.4
 */
class core_data_sources_t
	{
	public :
		core_data_sources_t(
			outliving_reference_t< stats::repository_t > ds_repository,
			impl::mbox_core_t & mbox_repository,
			so_5::environment_infrastructure_t & infrastructure )
			:	m_mbox_repository( std::move(ds_repository), mbox_repository )
			,	m_coop_repository( std::move(ds_repository), infrastructure )
			,	m_timer_thread( std::move(ds_repository), infrastructure )
			{}

	private :
		//! Data source for mboxes repository.
		stats::impl::ds_mbox_core_stats_t m_mbox_repository;
		//! Data source for cooperations repository.
		stats::impl::ds_agent_core_stats_t m_coop_repository;
		//! Data source for timer thread.
		stats::impl::ds_timer_thread_stats_t m_timer_thread;
	};

/*!
 * \brief Helper function for creation of appropriate manager
 * object if necessary.
 *
 * \since
 * v.5.5.18
 */
queue_locks_defaults_manager_unique_ptr_t
ensure_locks_defaults_manager_exists(
	//! The current value. Note: can be nullptr.
	queue_locks_defaults_manager_unique_ptr_t current )
	{
		queue_locks_defaults_manager_unique_ptr_t result( std::move(current) );

		if( !result )
			result = make_defaults_manager_for_combined_locks();

		return result;
	}

//
// default_event_queue_hook_t
//
/*!
 * \brief Default implementation of event_queue_hook.
 *
 * Do nothing.
 *
 * \since
 * v.5.5.24
 */
class default_event_queue_hook_t final : public event_queue_hook_t
	{
	public :
		SO_5_NODISCARD
		event_queue_t *
		on_bind(
			agent_t * /*agent*/,
			event_queue_t * original_queue ) noexcept override
			{
				return original_queue;
			}

		void
		on_unbind(
			agent_t * /*agent*/,
			event_queue_t * /*queue*/ ) noexcept override
			{
			}
	};

/*!
 * \brief Helper function for creation of appropriate event_queue_hook
 * object if necessary.
 *
 * \since
 * v.5.5.24
 */
SO_5_NODISCARD
event_queue_hook_unique_ptr_t
ensure_event_queue_hook_exists(
	//! The current value. Note: can be nullptr.
	event_queue_hook_unique_ptr_t current )
	{
		event_queue_hook_unique_ptr_t result( std::move(current) );

		if( !result )
			result = make_event_queue_hook< default_event_queue_hook_t >(
					&event_queue_hook_t::default_deleter );

		return result;
	}

} /* namespace anonymous */

//
// environment_t::internals_t
//
/*!
 * \since
 * v.5.5.0
 *
 * \brief Internal details of SObjectizer Environment object.
 */
struct environment_t::internals_t
{
	/*!
	 * \since
	 * v.5.5.0
	 *
	 * \brief Error logger object for this environment.
	 *
	 * \attention Must be the first attribute of the object!
	 * It must be created and initilized first and destroyed last.
	 */
	error_logger_shptr_t m_error_logger;

	/*!
	 * \brief Holder of stuff related to message delivery tracing.
	 *
	 * \attention This field must be declared and initialized
	 * before m_mbox_core because a reference to that object will be passed
	 * to the constructor of m_mbox_core.
	 *
	 * \since
	 * v.5.5.22
	 */
	so_5::msg_tracing::impl::std_holder_t m_msg_tracing_stuff;

	//! An utility for mboxes.
	impl::mbox_core_ref_t m_mbox_core;

	/*!
	 * \brief A repository of stop_guards.
	 *
	 * \since
	 * v.5.5.19.2
	 */
	impl::stop_guard_repository_t m_stop_guards;

	/*!
	 * \brief A specific infrastructure for environment.
	 *
	 * Note: infrastructure takes care about coop repository,
	 * timer threads/managers and default dispatcher.
	 *
	 * \since
	 * v.5.5.19
	 */
	environment_infrastructure_unique_ptr_t m_infrastructure;

	//! An utility for layers.
	impl::layer_core_t m_layer_core;

	/*!
	 * \brief An exception reaction for the whole SO Environment.
	 * \since
	 * v.5.3.0
	 */
	const exception_reaction_t m_exception_reaction;

	/*!
	 * \brief Is autoshutdown when there is no more cooperation disabled?
	 *
	 * \see environment_params_t::disable_autoshutdown()
	 *
	 * \since
	 * v.5.4.0
	 */
	const bool m_autoshutdown_disabled;

	/*!
	 * \brief A counter for automatically generated cooperation names.
	 *
	 * \since
	 * v.5.5.1
	 */
	std::atomic_uint_fast64_t m_autoname_counter = { 0 }; 

	/*!
	 * \brief Data sources for core objects.
	 *
	 * \attention This instance must be created after stats_controller
	 * and destroyed before it. Because of that m_core_data_sources declared
	 * after m_stats_controller and after all corresponding objects.
	 * NOTE: since v.5.5.19 stats_controller and stats_repository are parts
	 * of environment_infrastructure. Because of that m_core_data_sources
	 * declared and created after m_infrastructure.
	 * 
	 * \since
	 * v.5.5.4
	 */
	core_data_sources_t m_core_data_sources;

	/*!
	 * \brief Work thread activity tracking for the whole Environment.
	 * \since
	 * v.5.5.18
	 */
	work_thread_activity_tracking_t m_work_thread_activity_tracking;

	/*!
	 * \brief Manager for defaults of queue locks.
	 *
	 * \since
	 * v.5.5.18
	 */
	queue_locks_defaults_manager_unique_ptr_t m_queue_locks_defaults_manager;

	/*!
	 * \brief Actual event_queue_hook.
	 *
	 * \note
	 * If there is no event_queue_hook in environment_params_t then
	 * an instance of default_event_queue_hook_t will be created and used.
	 *
	 * \since
	 * v.5.5.24
	 */
	event_queue_hook_unique_ptr_t m_event_queue_hook;

	/*!
	 * \brief Lock object for protection of exception logger object.
	 *
	 * \note
	 * Manipulations with m_event_exception_logger are performed
	 * only under that lock.
	 *
	 * \since
	 * v.5.6.0
	 */
	std::mutex m_event_exception_logger_lock;

	/*!
	 * \brief Logger for exceptions thrown from event-handlers.
	 *
	 * \since
	 * v.5.6.0
	 */
	event_exception_logger_unique_ptr_t m_event_exception_logger;

	//! Constructor.
	internals_t(
		environment_t & env,
		environment_params_t && params )
		:	m_error_logger( params.so5__error_logger() )
		,	m_msg_tracing_stuff{
				params.so5__giveout_message_delivery_tracer_filter(),
				params.so5__giveout_message_delivery_tracer() }
		,	m_mbox_core(
				new impl::mbox_core_t{
						outliving_mutable( m_msg_tracing_stuff ) } )
		,	m_infrastructure(
				(params.infrastructure_factory())(
					env,
					params,
					// A special mbox for distributing monitoring information
					// must be created and passed to stats_controller.
					m_mbox_core->create_mbox(env) ) )
		,	m_layer_core(
				env,
				params.so5__layers_map() )
		,	m_exception_reaction( params.exception_reaction() )
		,	m_autoshutdown_disabled( params.autoshutdown_disabled() )
		,	m_core_data_sources(
				outliving_mutable(m_infrastructure->stats_repository()),
				*m_mbox_core,
				*m_infrastructure )
		,	m_work_thread_activity_tracking(
				params.work_thread_activity_tracking() )
		,	m_queue_locks_defaults_manager(
				ensure_locks_defaults_manager_exists(
					params.so5__giveout_queue_locks_defaults_manager() ) )
		,	m_event_queue_hook(
				ensure_event_queue_hook_exists(
					params.so5__giveout_event_queue_hook() ) )
		,	m_event_exception_logger{
				params.so5__giveout_event_exception_logger() }
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
environment_t::create_mbox()
{
	return m_impl->m_mbox_core->create_mbox( *this );
}

mbox_t
environment_t::create_mbox(
	nonempty_name_t nonempty_name )
{
	return m_impl->m_mbox_core->create_mbox( *this, std::move(nonempty_name) );
}

mchain_t
environment_t::create_mchain(
	const mchain_params_t & params )
{
	return m_impl->m_mbox_core->create_mchain( *this, params );
}

void
environment_t::install_exception_logger(
	event_exception_logger_unique_ptr_t logger )
{
	if( logger )
	{
		std::lock_guard< std::mutex > lock{
				m_impl->m_event_exception_logger_lock };

		using std::swap;
		swap( m_impl->m_event_exception_logger, logger );

		m_impl->m_event_exception_logger->on_install( std::move( logger ) );
	}
}

coop_unique_ptr_t
environment_t::create_coop(
	nonempty_name_t name )
{
	return create_coop( std::move(name), so_make_default_disp_binder() );
}

coop_unique_ptr_t
environment_t::create_coop(
	autoname_indicator_t indicator() )
{
	return create_coop( indicator, so_make_default_disp_binder() );
}

coop_unique_ptr_t
environment_t::create_coop(
	nonempty_name_t name,
	disp_binder_shptr_t disp_binder )
{
	return coop_unique_ptr_t( new coop_t(
			std::move(name), std::move(disp_binder), self_ref() ) );
}

coop_unique_ptr_t
environment_t::create_coop(
	autoname_indicator_t (*)(),
	disp_binder_shptr_t disp_binder )
{
	auto counter = ++(m_impl->m_autoname_counter);
	nonempty_name_t name( "__so5_autoname_" + std::to_string(counter) + "__" );
	return coop_unique_ptr_t( new coop_t(
			std::move(name),
			std::move(disp_binder),
			self_ref() ) );
}

void
environment_t::register_coop(
	coop_unique_ptr_t agent_coop )
{
	m_impl->m_infrastructure->register_coop( std::move( agent_coop ) );
}

void
environment_t::deregister_coop(
	nonempty_name_t name,
	int reason )
{
	m_impl->m_infrastructure->deregister_coop(
			std::move(name), coop_dereg_reason_t( reason ) );
}

so_5::timer_id_t
environment_t::so_schedule_timer(
	const low_level_api::schedule_timer_params_t params )
{
	// Since v.5.5.21 pause and period should be checked for negative
	// values.
	using duration = std::chrono::steady_clock::duration;
	if( params.m_pause < duration::zero() )
		SO_5_THROW_EXCEPTION(
				so_5::rc_negative_value_for_pause,
				"an attempt to call schedule_timer() with negative pause value" );
	if( params.m_period < duration::zero() )
		SO_5_THROW_EXCEPTION(
				so_5::rc_negative_value_for_period,
				"an attempt to call schedule_timer() with negative period value" );

	// If it is a mutable message then there must be some restrictions:
	if( message_mutability_t::mutable_message == message_mutability(params.m_msg) )
	{
		// Mutable message can be sent only as delayed message.
		if( std::chrono::steady_clock::duration::zero() != params.m_period )
			SO_5_THROW_EXCEPTION(
					so_5::rc_mutable_msg_cannot_be_periodic,
					"unable to schedule periodic timer for mutable message,"
					" msg_type=" + std::string(params.m_msg_type.name()) );
		// Mutable message can't be passed to MPMC-mbox.
		else if( mbox_type_t::multi_producer_multi_consumer == params.m_mbox->type() )
			SO_5_THROW_EXCEPTION(
					so_5::rc_mutable_msg_cannot_be_delivered_via_mpmc_mbox,
					"unable to schedule timer for mutable message and "
					"MPMC mbox, msg_type=" + std::string(params.m_msg_type.name()) );
	}

	return m_impl->m_infrastructure->schedule_timer(
			params.m_msg_type,
			params.m_msg,
			params.m_mbox,
			params.m_pause,
			params.m_period );
}

void
environment_t::so_single_timer(
	const low_level_api::single_timer_params_t params )
{
	// Since v.5.5.21 pause should be checked for negative values.
	using duration = std::chrono::steady_clock::duration;
	if( params.m_pause < duration::zero() )
		SO_5_THROW_EXCEPTION(
				so_5::rc_negative_value_for_pause,
				"an attempt to call single_timer() with negative pause value" );

	// Mutable message can't be passed to MPMC-mbox.
	if( message_mutability_t::mutable_message == message_mutability(params.m_msg) &&
			mbox_type_t::multi_producer_multi_consumer == params.m_mbox->type() )
		SO_5_THROW_EXCEPTION(
				so_5::rc_mutable_msg_cannot_be_delivered_via_mpmc_mbox,
				"unable to schedule single timer for mutable message and "
				"MPMC mbox, msg_type=" + std::string(params.m_msg_type.name()) );

	m_impl->m_infrastructure->single_timer(
			params.m_msg_type,
			params.m_msg,
			params.m_mbox,
			params.m_pause );
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
	// Since v.5.5.19.2 there is a new shutdown procedure:
	const auto action = m_impl->m_stop_guards.initiate_stop();
	if( impl::stop_guard_repository_t::action_t::do_actual_stop == action )
		m_impl->m_infrastructure->stop();
}

void
environment_t::call_exception_logger(
	const std::exception & event_exception,
	const std::string & coop_name )
{
	std::lock_guard< std::mutex > lock{ m_impl->m_event_exception_logger_lock };

	m_impl->m_event_exception_logger->log_exception( event_exception, coop_name );
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
	return m_impl->m_infrastructure->stats_controller();
}

stats::repository_t &
environment_t::stats_repository()
{
	return m_impl->m_infrastructure->stats_repository();
}

work_thread_activity_tracking_t
environment_t::work_thread_activity_tracking() const
{
	return m_impl->m_work_thread_activity_tracking;
}

disp_binder_shptr_t
environment_t::so_make_default_disp_binder()
{
	return m_impl->m_infrastructure->make_default_disp_binder();
}

bool
environment_t::autoshutdown_disabled() const
{
	return m_impl->m_autoshutdown_disabled;
}

mbox_t
environment_t::do_make_custom_mbox(
	custom_mbox_details::creator_iface_t & creator )
{
	return m_impl->m_mbox_core->create_custom_mbox( *this, creator );
}

stop_guard_t::setup_result_t
environment_t::setup_stop_guard(
	stop_guard_shptr_t guard,
	stop_guard_t::what_if_stop_in_progress_t reaction_on_stop_in_progress )
{
	const auto result = m_impl->m_stop_guards.setup_guard( std::move(guard) );
	if( stop_guard_t::setup_result_t::stop_already_in_progress == result 
			&& stop_guard_t::what_if_stop_in_progress_t::throw_exception ==
					reaction_on_stop_in_progress )
	{
		SO_5_THROW_EXCEPTION(
				rc_cannot_set_stop_guard_when_stop_is_started,
				"stop_guard can't be set because the stop operation is "
				"already in progress" );
	}

	return result;
}

void
environment_t::remove_stop_guard(
	stop_guard_shptr_t guard )
{
	const auto action = m_impl->m_stop_guards.remove_guard( std::move(guard) );
	if( impl::stop_guard_repository_t::action_t::do_actual_stop == action )
		m_impl->m_infrastructure->stop();
}

void
environment_t::change_message_delivery_tracer_filter(
	so_5::msg_tracing::filter_shptr_t filter )
{
	if( !m_impl->m_msg_tracing_stuff.is_msg_tracing_enabled() )
		SO_5_THROW_EXCEPTION(
				rc_msg_tracing_disabled,
				"msg_tracing's filter can't be changed when msg_tracing "
				"is disabled" );

	m_impl->m_msg_tracing_stuff.change_filter( std::move(filter) );
}

void
environment_t::impl__run_stats_controller_and_go_further()
{
	impl::run_stage(
			"run_stats_controller",
			[] {
				/* there is no need to turn_on controller automatically */
			},
			[this] { m_impl->m_infrastructure->stats_controller().turn_off(); },
			[this] { impl__run_layers_and_go_further(); } );
}

void
environment_t::impl__run_layers_and_go_further()
{
	impl::run_stage(
			"run_layers",
			[this] { m_impl->m_layer_core.start(); },
			[this] { m_impl->m_layer_core.finish(); },
			[this] { impl__run_infrastructure(); } );
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
					env.make_agent< a_empty_agent_t >() );
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
environment_t::impl__run_infrastructure()
{
	m_impl->m_infrastructure->launch( 
		[this]()
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
		} );
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
	m_env.m_impl->m_infrastructure->ready_to_deregister_notify( coop );
}

void
internal_env_iface_t::final_deregister_coop(
	const std::string & coop_name )
{
	bool any_cooperation_alive = 
			m_env.m_impl->m_infrastructure->final_deregister_coop( coop_name );

	if( !any_cooperation_alive && !m_env.m_impl->m_autoshutdown_disabled )
		m_env.stop();
}

bool
internal_env_iface_t::is_msg_tracing_enabled() const
{
	return m_env.m_impl->m_msg_tracing_stuff.is_msg_tracing_enabled();
}

so_5::msg_tracing::holder_t &
internal_env_iface_t::msg_tracing_stuff() const
{
	if( !is_msg_tracing_enabled() )
		SO_5_THROW_EXCEPTION( rc_msg_tracing_disabled,
				"msg_tracer cannot be accessed because msg_tracing is disabled" );

	return m_env.m_impl->m_msg_tracing_stuff;
}

so_5::disp::mpsc_queue_traits::lock_factory_t
internal_env_iface_t::default_mpsc_queue_lock_factory() const
{
	return m_env.m_impl->m_queue_locks_defaults_manager->
			mpsc_queue_lock_factory();
}

so_5::disp::mpmc_queue_traits::lock_factory_t
internal_env_iface_t::default_mpmc_queue_lock_factory() const
{
	return m_env.m_impl->m_queue_locks_defaults_manager->
			mpmc_queue_lock_factory();
}

SO_5_NODISCARD
event_queue_t *
internal_env_iface_t::event_queue_on_bind(
	agent_t * agent,
	event_queue_t * original_queue ) noexcept
{
	return m_env.m_impl->m_event_queue_hook->on_bind( agent, original_queue );
}

void
internal_env_iface_t::event_queue_on_unbind(
	agent_t * agent,
	event_queue_t * queue ) noexcept
{
	m_env.m_impl->m_event_queue_hook->on_unbind( agent, queue );
}

} /* namespace impl */

} /* namespace so_5 */


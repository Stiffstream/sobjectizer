/*
	SObjectizer 5.
*/

#include <exception>

#include <so_5/h/exception.hpp>
#include <so_5/h/ret_code.hpp>

#include <so_5/rt/impl/h/so_environment_impl.hpp>

namespace so_5
{

namespace rt
{

namespace impl
{

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

so_environment_impl_t::so_environment_impl_t(
	so_environment_params_t && so_environment_params,
	so_environment_t & public_so_environment )
//FIXME: error_logger object must be taken from so_environment_params!
	:	m_error_logger( create_stderr_logger() )
	,	m_mbox_core( new mbox_core_t() )
	,	m_agent_core(
			public_so_environment,
			std::move( so_environment_params.m_coop_listener ) )
	,	m_disp_core(
			public_so_environment,
			so_environment_params.named_dispatcher_map(),
			std::move( so_environment_params.m_event_exception_logger ) )
	,	m_layer_core(
			so_environment_params.so_layers_map(),
			&public_so_environment )
	,	m_public_so_environment( public_so_environment )
	,	m_timer_thread(
			create_appropriate_timer_thread(
					m_error_logger,
					so_environment_params.m_timer_thread_factory ) )
	,	m_exception_reaction( so_environment_params.exception_reaction() )
	,	m_autoshutdown_disabled( so_environment_params.autoshutdown_disabled() )
{
}

so_environment_impl_t::~so_environment_impl_t()
{
}

so_5::timer_id_t
so_environment_impl_t::schedule_timer(
	const std::type_index & type_wrapper,
	const message_ref_t & msg,
	const mbox_ref_t & mbox,
	unsigned int delay_msec,
	unsigned int period_msec )
{
	return m_timer_thread->schedule(
			type_wrapper,
			mbox,
			msg,
			std::chrono::milliseconds( delay_msec ),
			std::chrono::milliseconds( period_msec ) );
}

void
so_environment_impl_t::single_timer(
	const std::type_index & type_wrapper,
	const message_ref_t & msg,
	const mbox_ref_t & mbox,
	unsigned int delay_msec )
{
	return m_timer_thread->schedule_anonymous(
			type_wrapper,
			mbox,
			msg,
			std::chrono::milliseconds( delay_msec ),
			std::chrono::milliseconds::zero() );
}

so_layer_t *
so_environment_impl_t::query_layer(
	const std::type_index & type ) const
{
	return m_layer_core.query_layer( type );
}

void
so_environment_impl_t::add_extra_layer(
	const std::type_index & type,
	const so_layer_ref_t & layer )
{
	m_layer_core.add_extra_layer( type, layer );
}

void
so_environment_impl_t::run(
	so_environment_t & env )
{
	try
	{
		run_layers_and_go_further( env );
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
so_environment_impl_t::stop()
{
	// Sends shutdown signal for all agents.
	m_agent_core.start_deregistration();
}

so_environment_t &
so_environment_impl_t::query_public_so_environment()
{
	return m_public_so_environment;
}

exception_reaction_t
so_environment_impl_t::exception_reaction() const
{
	return m_exception_reaction;
}

error_logger_t &
so_environment_impl_t::error_logger() const
{
	return *m_error_logger;
}

void
so_environment_impl_t::run_layers_and_go_further(
	so_environment_t & env )
{
	do_run_stage(
			"run_layers",
			[this] { m_layer_core.start(); },
			[this] { m_layer_core.finish(); },
			[this, &env] { run_dispatcher_and_go_further( env ); } );
}

void
so_environment_impl_t::run_dispatcher_and_go_further(
	so_environment_t & env )
{
	do_run_stage(
			"run_dispatcher",
			[this] { m_disp_core.start(); },
			[this] { m_disp_core.finish(); },
			[this, &env] { run_timer_and_go_further( env ); } );
}

void
so_environment_impl_t::run_timer_and_go_further(
	so_environment_t & env )
{
	do_run_stage(
			"run_timer",
			[this] { m_timer_thread->start(); },
			[this] { m_timer_thread->finish(); },
			[this, &env] { run_agent_core_and_go_further( env ); } );
}

void
so_environment_impl_t::run_agent_core_and_go_further(
	so_environment_t & env )
{
	do_run_stage(
			"run_agent_core",
			[this] { m_agent_core.start(); },
			[this] { m_agent_core.finish(); },
			[this, &env] { run_user_supplied_init_and_wait_for_stop( env ); } );
}

namespace autoshutdown_guard
{
	//! An empty agent for the special cooperation for protection of
	//! init function from autoshutdown feature.
	class a_empty_agent_t : public agent_t
	{
		public :
			a_empty_agent_t( so_environment_t & env )
				:	agent_t( env )
			{}
	};

	void
	register_init_guard_cooperation(
		so_environment_t & env,
		bool autoshutdown_disabled )
	{
		if( !autoshutdown_disabled )
			env.register_agent_as_coop(
					"__so_5__init_autoshutdown_guard__",
					new a_empty_agent_t( env ) );
	}

	void
	deregistr_init_guard_cooperation(
		so_environment_t & env,
		bool autoshutdown_disabled )
	{
		if( !autoshutdown_disabled )
			env.deregister_coop(
					"__so_5__init_autoshutdown_guard__",
					dereg_reason::normal );
	}
}

void
so_environment_impl_t::run_user_supplied_init_and_wait_for_stop(
	so_environment_t & env )
{
	try
	{
		// init method must be protected from autoshutdown feature.
		autoshutdown_guard::register_init_guard_cooperation(
				m_public_so_environment,
				m_autoshutdown_disabled );

		// Initilizing environment.
		env.init();

		// Protection is no more needed.
		autoshutdown_guard::deregistr_init_guard_cooperation(
				m_public_so_environment,
				m_autoshutdown_disabled );

		m_agent_core.wait_for_start_deregistration();
	}
	catch( const std::exception & ex )
	{
		env.stop();
		m_agent_core.wait_for_start_deregistration();

		throw;
	}
}

void
so_environment_impl_t::do_run_stage(
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

} /* namespace impl */

} /* namespace rt */

} /* namespace so_5 */


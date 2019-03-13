/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Default implementation of multithreaded environment infrastructure.
 *
 * \since
 * v.5.5.19
 */

#include <so_5/impl/mt_env_infrastructure.hpp>

#include <so_5/impl/run_stage.hpp>

#include <so_5/environment.hpp>
#include <so_5/send_functions.hpp>

#include <so_5/disp/one_thread/pub.hpp>

namespace so_5 {

namespace env_infrastructures {

namespace default_mt {

namespace impl {

//
// coop_repo_t
//
coop_repo_t::coop_repo_t(
	outliving_reference_t< environment_t > env,
	coop_listener_unique_ptr_t coop_listener )
	:	coop_repository_basis_t( env, std::move(coop_listener) )
	{}

void
coop_repo_t::start()
{
	// mchain for final coop deregs must be created.
	m_final_dereg_chain = environment().create_mchain(
			make_unlimited_mchain_params().disable_msg_tracing() );
	// A separate thread for doing the final dereg must be started.
	m_final_dereg_thread = std::thread{ [this] {
		// Process dereg demands until chain will be closed.
		receive( from( m_final_dereg_chain ),
			[]( coop_shptr_t coop ) {
//FIXME: final_deregister_coop should be called!
(void)coop;
//				coop_t::so_call_final_deregister_coop( coop );
			} );
	} };
}

void
coop_repo_t::finish()
{
	// Deregistration of all cooperations should be initiated.
	deregister_all_coop();

	// Deregistration of all cooperations should be finished.
	wait_all_coop_to_deregister();

	// Notify a dedicated thread and wait while it will be stopped.
	close_retain_content( m_final_dereg_chain );
	m_final_dereg_thread.join();
}

void
coop_repo_t::ready_to_deregister_notify(
	coop_shptr_t coop )
{
	so_5::send< coop_shptr_t >( m_final_dereg_chain, std::move(coop) );
}

bool
coop_repo_t::final_deregister_coop(
	coop_shptr_t coop )
{
	const auto result =
			coop_repository_basis_t::final_deregister_coop( std::move(coop) );

	if( result.m_total_deregistration_completed )
		m_deregistration_finished_cond.notify_one();

	return result.m_has_live_coop;
}

void
coop_repo_t::start_deregistration()
{
	const auto result = coop_repository_basis_t::initiate_deregistration();

	if( coop_repository_basis_t::initiate_deregistration_result_t
			::initiated_first_time == result )
		m_deregistration_started_cond.notify_one();
}

void
coop_repo_t::wait_for_start_deregistration()
{
//FIXME: implement this!
#if 0
	std::unique_lock< std::mutex > lck( this->lock() );

	m_deregistration_started_cond.wait( lck,
			[this] { return m_deregistration_started; } );
#endif
}

void
coop_repo_t::wait_all_coop_to_deregister()
{
//FIXME: implement this!
#if 0
	std::unique_lock< std::mutex > lck( this->lock() );

	// Must wait for a signal is there are cooperations in
	// the deregistration process.
	m_deregistration_finished_cond.wait( lck,
			[this] { return m_deregistered_coop.empty(); } );
#endif
}

environment_infrastructure_t::coop_repository_stats_t
coop_repo_t::query_stats()
{
	const auto final_dereg_coops = m_final_dereg_chain->size();

	const auto basis_stats = coop_repository_basis_t::query_stats();

	return {
			basis_stats.m_registered_coop_count,
			basis_stats.m_deregistered_coop_count,
			basis_stats.m_total_agent_count,
			final_dereg_coops
		};
}

//
// mt_env_infrastructure_t
//
mt_env_infrastructure_t::mt_env_infrastructure_t(
	environment_t & env,
	so_5::disp::one_thread::disp_params_t default_disp_params,
	timer_thread_unique_ptr_t timer_thread,
	coop_listener_unique_ptr_t coop_listener,
	mbox_t stats_distribution_mbox )
	:	m_env( env )
	,	m_default_dispatcher_params{ std::move(default_disp_params) }
	,	m_timer_thread( std::move(timer_thread) )
	,	m_coop_repo( outliving_mutable(env), std::move(coop_listener) )
	,	m_stats_controller( std::move(stats_distribution_mbox) )
	{
	}

void
mt_env_infrastructure_t::launch( env_init_t init_fn )
	{
		run_default_dispatcher_and_go_further( std::move(init_fn) );
	}

void
mt_env_infrastructure_t::stop()
	{
		// Sends shutdown signal for all agents.
		m_coop_repo.start_deregistration();
	}

SO_5_NODISCARD
coop_unique_ptr_t
mt_env_infrastructure_t::make_coop(
	coop_handle_t parent,
	disp_binder_shptr_t default_binder )
{
	return m_coop_repo.make_coop( std::move(parent), std::move(default_binder) );
}

coop_handle_t
mt_env_infrastructure_t::register_coop(
	coop_unique_ptr_t coop )
	{
		return m_coop_repo.register_coop( std::move(coop) );
	}

void
mt_env_infrastructure_t::deregister_coop(
	coop_handle_t coop,
	coop_dereg_reason_t dereg_reason )
	{
		m_coop_repo.deregister_coop( std::move(coop), dereg_reason );
	}

void
mt_env_infrastructure_t::ready_to_deregister_notify(
	coop_shptr_t coop )
	{
		m_coop_repo.ready_to_deregister_notify( std::move(coop) );
	}

bool
mt_env_infrastructure_t::final_deregister_coop(
	coop_shptr_t coop )
	{
		return m_coop_repo.final_deregister_coop( std::move(coop) );
	}

so_5::timer_id_t
mt_env_infrastructure_t::schedule_timer(
	const std::type_index & type_wrapper,
	const message_ref_t & msg,
	const mbox_t & mbox,
	std::chrono::steady_clock::duration pause,
	std::chrono::steady_clock::duration period )
	{
		return m_timer_thread->schedule(
				type_wrapper,
				mbox,
				msg,
				pause,
				period );
	}

void
mt_env_infrastructure_t::single_timer(
	const std::type_index & type_wrapper,
	const message_ref_t & msg,
	const mbox_t & mbox,
	std::chrono::steady_clock::duration pause )
	{
		m_timer_thread->schedule_anonymous(
				type_wrapper,
				mbox,
				msg,
				pause,
				std::chrono::milliseconds::zero() );
	}

::so_5::stats::controller_t &
mt_env_infrastructure_t::stats_controller() noexcept
	{
		return m_stats_controller;
	}

::so_5::stats::repository_t &
mt_env_infrastructure_t::stats_repository() noexcept
	{
		return m_stats_controller;
	}

environment_infrastructure_t::coop_repository_stats_t
mt_env_infrastructure_t::query_coop_repository_stats()
	{
		return m_coop_repo.query_stats();
	}

timer_thread_stats_t
mt_env_infrastructure_t::query_timer_thread_stats()
	{
		return m_timer_thread->query_stats();
	}

disp_binder_shptr_t
mt_env_infrastructure_t::make_default_disp_binder()
	{
		return m_default_dispatcher.binder();
	}

void
mt_env_infrastructure_t::run_default_dispatcher_and_go_further(
	env_init_t init_fn )
	{
		::so_5::impl::run_stage(
				"run_default_dispatcher",
				[this] {
					// Default dispatcher should be created.
					m_default_dispatcher = so_5::disp::one_thread::make_dispatcher(
							m_env,
							std::string{ "DEFAULT" },
							m_default_dispatcher_params );
				},
				[this] {
					// Default dispatcher is no more needed.
					m_default_dispatcher.reset();
				},
				[this, init_fn] {
					run_timer_thread_and_go_further( std::move(init_fn) );
				} );
	}

void
mt_env_infrastructure_t::run_timer_thread_and_go_further(
	env_init_t init_fn )
	{
		::so_5::impl::run_stage(
				"run_timer",
				[this] { m_timer_thread->start(); },
				[this] { m_timer_thread->finish(); },
				[this, init_fn] {
					run_agent_core_and_go_further( std::move(init_fn) );
				} );
	}

void
mt_env_infrastructure_t::run_agent_core_and_go_further(
	env_init_t init_fn )
	{
		::so_5::impl::run_stage(
				"run_agent_core",
				[this] { m_coop_repo.start(); },
				[this] { m_coop_repo.finish(); },
				[this, init_fn] {
					run_user_supplied_init_and_wait_for_stop( std::move(init_fn) );
				} );
	}

void
mt_env_infrastructure_t::run_user_supplied_init_and_wait_for_stop(
	env_init_t init_fn )
	{
		so_5::details::do_with_rollback_on_exception(
			[this, &init_fn]
			{
				init_fn();
				m_coop_repo.wait_for_start_deregistration();
			},
			[this]
			{
				stop();
				m_coop_repo.wait_for_start_deregistration();
			} );
	}

} /* namespace impl */

//
// factory
//
SO_5_FUNC environment_infrastructure_factory_t
factory()
	{
		return [](
				environment_t & env,
				environment_params_t & params,
				mbox_t stats_distribution_mbox )
		{
			// Timer thread is necessary for that environment.
			auto timer =
					so_5::internal_timer_helpers::create_appropriate_timer_thread(
							params.so5__error_logger(),
							params.so5__giveout_timer_thread_factory() );

			// Now the environment object can be created.
			auto obj = new impl::mt_env_infrastructure_t(
					env,
					params.default_disp_params(),
					std::move(timer),
					params.so5__giveout_coop_listener(),
					std::move(stats_distribution_mbox) );

			return environment_infrastructure_unique_ptr_t(
					obj,
					environment_infrastructure_t::default_deleter() );
		};
	}

} /* namespace default_mt */

} /* namespace env_infrastructures */

} /* namespace so_5 */


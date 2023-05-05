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
#include <so_5/impl/internal_env_iface.hpp>

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
	,	m_final_dereg_thread_shutdown_flag{ false }
	{}

void
coop_repo_t::start()
{
	// A separate thread for doing the final dereg must be started.
	m_final_dereg_thread = std::thread{ [this] { final_dereg_thread_body(); } };
}

void
coop_repo_t::finish()
{
	// Deregistration of all cooperations should be initiated.
	deregister_all_coop();

	// Deregistration of all cooperations should be finished.
	wait_all_coop_to_deregister();

	// Notify a dedicated thread and wait while it will be stopped.
	{
		std::lock_guard< std::mutex > lock{ m_final_dereg_chain_lock };
		m_final_dereg_thread_shutdown_flag = true;
		m_final_dereg_chain_cond.notify_one();
	}
	m_final_dereg_thread.join();
}

void
coop_repo_t::ready_to_deregister_notify(
	coop_shptr_t coop )
{
	std::lock_guard< std::mutex > lck{ m_final_dereg_chain_lock };

	// Update the final_dereg_chain.
	m_final_dereg_chain.append( std::move(coop) );

	if( 1u == m_final_dereg_chain.size() )
	{
		// Final deregistration thread may wait, have to wake it up.
		m_final_dereg_chain_cond.notify_one();
	}
}

bool
coop_repo_t::final_deregister_coop(
	coop_shptr_t coop ) noexcept
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
	const auto result = coop_repository_basis_t::try_switch_to_shutdown();

	if( coop_repository_basis_t::try_switch_to_shutdown_result_t
			::switched == result )
		m_deregistration_started_cond.notify_one();
}

void
coop_repo_t::wait_for_start_deregistration()
{
	std::unique_lock< std::mutex > lck{ m_lock };

	m_deregistration_started_cond.wait( lck,
			[this] { return status_t::normal != m_status; } );
}

void
coop_repo_t::wait_all_coop_to_deregister()
{
	std::unique_lock< std::mutex > lck{ m_lock };

	// Must wait for a signal is there are cooperations in
	// the deregistration process.
	m_deregistration_finished_cond.wait( lck,
			[this] { return 0u == m_total_coops; } );
}

environment_infrastructure_t::coop_repository_stats_t
coop_repo_t::query_stats()
{
	const auto final_dereg_coops = [this]() {
			std::lock_guard< std::mutex > lck{ m_final_dereg_chain_lock };
			return m_final_dereg_chain.size();
		}();

	const auto basis_stats = coop_repository_basis_t::query_stats();

	return {
			basis_stats.m_total_coop_count,
			basis_stats.m_total_agent_count,
			final_dereg_coops
		};
}

void
coop_repo_t::final_dereg_thread_body()
{
	std::unique_lock< std::mutex > lck{ m_final_dereg_chain_lock };

	for( bool should_continue = true; should_continue; )
	{
		bool should_wait = true;

		// If there are some waiting coops they have to be processed even
		// if m_final_dereg_thread_shutdown_flag is set.
		if( !m_final_dereg_chain.empty() )
		{
			// There are some coops to be deregistered.
			process_current_final_dereg_chain( lck );

			// Because the processing takes some time there is no need
			// to sleep before new check for m_final_dereg_chain.
			should_wait = false;
		}

		if( m_final_dereg_thread_shutdown_flag )
		{
			// It's time to finish the work.
			should_continue = false;
		}
		else if( should_wait )
		{
			// No coops to deregister. Have to wait.
			m_final_dereg_chain_cond.wait( lck );
		}
	}
}

void
coop_repo_t::process_current_final_dereg_chain(
	std::unique_lock< std::mutex > & lck ) noexcept
{
	//
	// NOTE: don't expect exceptions here.
	//

	// There are some coops to be deregistered.
	// Have to extract the current value of final dereg chain from
	// the coop_repo instance.
	coop_shptr_t head = m_final_dereg_chain.giveout_current_chain();

	// All following actions has to be performed on unlocked mutex.
	lck.unlock();

	// Do final_deregister_coop for every item in the chain
	// one by one.
	so_5::impl::process_final_dereg_chain( std::move(head) );

	// Have to reacquire the lock back.
	lck.lock();
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
mt_env_infrastructure_t::stop() noexcept
	{
		// Sends shutdown signal for all agents.
		m_coop_repo.start_deregistration();
	}

[[nodiscard]]
coop_unique_holder_t
mt_env_infrastructure_t::make_coop(
	coop_handle_t parent,
	disp_binder_shptr_t default_binder )
	{
		return m_coop_repo.make_coop( std::move(parent), std::move(default_binder) );
	}

coop_handle_t
mt_env_infrastructure_t::register_coop(
	coop_unique_holder_t coop )
	{
		return m_coop_repo.register_coop( std::move(coop) );
	}

void
mt_env_infrastructure_t::ready_to_deregister_notify(
	coop_shptr_t coop ) noexcept
	{
		m_coop_repo.ready_to_deregister_notify( std::move(coop) );
	}

bool
mt_env_infrastructure_t::final_deregister_coop(
	coop_shptr_t coop ) noexcept
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
				so_5::impl::wrap_init_fn_call( std::move(init_fn) );
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
							params.so5_error_logger(),
							params.so5_giveout_timer_thread_factory() );

			// Now the environment object can be created.
			auto obj = new impl::mt_env_infrastructure_t(
					env,
					params.default_disp_params(),
					std::move(timer),
					params.so5_giveout_coop_listener(),
					std::move(stats_distribution_mbox) );

			return environment_infrastructure_unique_ptr_t(
					obj,
					environment_infrastructure_t::default_deleter() );
		};
	}

} /* namespace default_mt */

} /* namespace env_infrastructures */

} /* namespace so_5 */


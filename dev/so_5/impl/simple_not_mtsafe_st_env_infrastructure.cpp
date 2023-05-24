/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief A very simple not-multithreaded-safe single thread
 * environment infrastructure.
 *
 * \since v.5.5.19
 */

#include <so_5/impl/st_env_infrastructure_reuse.hpp>

#include <so_5/impl/run_stage.hpp>
#include <so_5/impl/internal_env_iface.hpp>

#include <so_5/impl/final_dereg_chain_helpers.hpp>

#include <so_5/disp/reuse/data_source_prefix_helpers.hpp>

#include <so_5/environment.hpp>
#include <so_5/env_infrastructures.hpp>

#include <so_5/details/at_scope_exit.hpp>
#include <so_5/details/sync_helpers.hpp>

namespace so_5 {

namespace env_infrastructures {

namespace simple_not_mtsafe {

namespace impl {

//! A short name for namespace with run-time stats stuff.
namespace stats = ::so_5::stats;

//! A short name for namespace with reusable stuff.
namespace reusable = ::so_5::env_infrastructures::st_reusable_stuff;

//
// shutdown_status_t
//
using shutdown_status_t = reusable::shutdown_status_t;

//
// event_queue_impl_t
//
/*!
 * \brief Implementation of event_queue interface for this type of
 * environment infrastructure.
 *
 * \since v.5.5.19
 */
class event_queue_impl_t final : public so_5::event_queue_t
	{
	public :
		//! Type for representation of statistical data for this event queue.
		struct stats_t
			{
				//! The current size of the demands queue.
				std::size_t m_demands_count;
			};

		void
		push( execution_demand_t demand ) override
			{
				m_demands.push_back( std::move(demand) );
			}

		/*!
		 * \note
		 * Delegates the work to the push() method.
		 */
		void
		push_evt_start( execution_demand_t demand ) override
			{
				this->push( std::move(demand) );
			}

		/*!
		 * \note
		 * Delegates the work to the push() method.
		 *
		 * \attention
		 * Terminates the whole application if the push() throws.
		 */
		void
		push_evt_finish( execution_demand_t demand ) noexcept override
			{
				this->push( std::move(demand) );
			}

		stats_t
		query_stats() const
			{
				return { m_demands.size() };
			}

		//! Type for result of extraction operation.
		enum class pop_result_t
			{
				extracted,
				empty_queue
			};

		pop_result_t
		pop( execution_demand_t & receiver ) noexcept
			{
				if( !m_demands.empty() )
					{
						receiver = std::move(m_demands.front());
						m_demands.pop_front();
						return pop_result_t::extracted;
					}

				return pop_result_t::empty_queue;
			}

	private :
		std::deque< execution_demand_t > m_demands;
	};

//
// coop_repo_t
//
/*!
 * \brief Implementation of coop_repository for
 * simple thread-safe single-threaded environment infrastructure.
 *
 * \since v.5.5.19
 */
using coop_repo_t = reusable::coop_repo_t;

//
// disp_ds_name_parts_t
//
/*!
 * \brief A special class for generation of names for dispatcher data sources.
 *
 * \since v.5.5.19
 */
struct disp_ds_name_parts_t
	{
		static constexpr const char *
		disp_type_part() noexcept { return "not_mtsafe_st_env"; }
	};

//
// default_dispatcher_t
//
/*!
 * \brief An implementation of dispatcher to be used in
 * places where default dispatcher is needed.
 *
 * \tparam Activity_Tracker a type of activity tracker to be used
 * for run-time statistics.
 *
 * \since v.5.5.19
 */
template< typename Activity_Tracker >
using default_dispatcher_t =
	reusable::default_dispatcher_t<
			event_queue_impl_t,
			Activity_Tracker,
			disp_ds_name_parts_t >;

//
// stats_controller_t
//
/*!
 * \brief Implementation of stats_controller for that type of
 * single-threaded environment.
 *
 * \since v.5.5.19
 */
using stats_controller_t =
	reusable::stats_controller_t< so_5::details::no_lock_holder_t >;

//
// env_infrastructure_t
//
/*!
 * \brief Default implementation of not-thread safe single-threaded environment
 * infrastructure.
 *
 * \attention
 * This class doesn't have any mutex inside.
 *
 * \tparam Activity_Tracker A type for tracking activity of main working thread.
 *
 * \since v.5.5.19
 */
template< typename Activity_Tracker >
class env_infrastructure_t
	: public environment_infrastructure_t
	{
	public :
		env_infrastructure_t(
			//! Environment to work in.
			environment_t & env,
			//! Factory for timer manager.
			timer_manager_factory_t timer_factory,
			//! Error logger necessary for timer_manager.
			error_logger_shptr_t error_logger,
			//! Cooperation action listener.
			coop_listener_unique_ptr_t coop_listener,
			//! Mbox for distribution of run-time stats.
			mbox_t stats_distribution_mbox );

		void
		launch( env_init_t init_fn ) override;

		void
		stop() noexcept override;

		[[nodiscard]]
		coop_unique_holder_t
		make_coop(
			coop_handle_t parent,
			disp_binder_shptr_t default_binder ) override;

		coop_handle_t
		register_coop(
			coop_unique_holder_t coop ) override;

		void
		ready_to_deregister_notify(
			coop_shptr_t coop ) noexcept override;

		bool
		final_deregister_coop(
			coop_shptr_t coop_name ) noexcept override;

		so_5::timer_id_t
		schedule_timer(
			const std::type_index & type_wrapper,
			const message_ref_t & msg,
			const mbox_t & mbox,
			std::chrono::steady_clock::duration pause,
			std::chrono::steady_clock::duration period ) override;

		void
		single_timer(
			const std::type_index & type_wrapper,
			const message_ref_t & msg,
			const mbox_t & mbox,
			std::chrono::steady_clock::duration pause ) override;

		stats::controller_t &
		stats_controller() noexcept override;

		stats::repository_t &
		stats_repository() noexcept override;

		so_5::environment_infrastructure_t::coop_repository_stats_t
		query_coop_repository_stats() override;

		timer_thread_stats_t
		query_timer_thread_stats() override;

		disp_binder_shptr_t
		make_default_disp_binder() override;

	private :
		environment_t & m_env;

		/*!
		 * \brief The chain of coops for the final deregistration.
		 *
		 * \since v.5.8.0
		 */
		so_5::impl::final_dereg_chain_holder_t m_final_dereg_chain;

		//! Status of shutdown procedure.
		shutdown_status_t m_shutdown_status{ shutdown_status_t::not_started };

		//! A collector for elapsed timers.
		reusable::direct_delivery_elapsed_timers_collector_t m_timers_collector;

		//! A timer manager to be used.
		timer_manager_unique_ptr_t m_timer_manager;

		//! Queue for execution_demands which must be handled on the main thread.
		event_queue_impl_t m_event_queue;

		//! Repository of registered coops.
		coop_repo_t m_coop_repo;

		//! Actual activity tracker for main working thread.
		Activity_Tracker m_activity_tracker;

		//! Dispatcher to be used as default dispatcher.
		/*!
		 * \note
		 * Has an actual value only inside launch() method.
		 */
		std::shared_ptr< default_dispatcher_t< Activity_Tracker > > m_default_disp;

		//! Stats controller for this environment.
		stats_controller_t m_stats_controller;

		void
		run_default_dispatcher_and_go_further(
			env_init_t init_fn );

		void
		run_user_supplied_init_and_do_main_loop(
			env_init_t init_fn );

		void
		run_main_loop() noexcept;

		void
		process_final_deregs_if_any() noexcept;

		void
		perform_shutdown_related_actions_if_needed() noexcept;

		void
		handle_expired_timers_if_any() noexcept;

		void
		try_handle_next_demand() noexcept;
	};

template< typename Activity_Tracker >
env_infrastructure_t< Activity_Tracker >::env_infrastructure_t(
	environment_t & env,
	timer_manager_factory_t timer_factory,
	error_logger_shptr_t error_logger,
	coop_listener_unique_ptr_t coop_listener,
	mbox_t stats_distribution_mbox )
	:	m_env( env )
	,	m_timer_manager(
			timer_factory(
				std::move(error_logger),
				outliving_mutable(m_timers_collector) ) )
	,	m_coop_repo( outliving_mutable(env), std::move(coop_listener) )
	,	m_stats_controller(
			std::move(stats_distribution_mbox),
			stats::impl::st_env_stuff::next_turn_mbox_t::make(m_env) )
	{}

template< typename Activity_Tracker >
void
env_infrastructure_t< Activity_Tracker >::launch( env_init_t init_fn )
	{
		run_default_dispatcher_and_go_further( std::move(init_fn) );
	}

template< typename Activity_Tracker >
void
env_infrastructure_t< Activity_Tracker >::stop() noexcept
	{
		if( shutdown_status_t::not_started == m_shutdown_status )
			{
				m_shutdown_status = shutdown_status_t::must_be_started;
			}
	}

template< typename Activity_Tracker >
coop_unique_holder_t
env_infrastructure_t< Activity_Tracker >::make_coop(
	coop_handle_t parent,
	disp_binder_shptr_t default_binder )
	{
		return m_coop_repo.make_coop(
				std::move(parent),
				std::move(default_binder) );
	}

template< typename Activity_Tracker >
coop_handle_t
env_infrastructure_t< Activity_Tracker >::register_coop(
	coop_unique_holder_t coop )
	{
		return m_coop_repo.register_coop( std::move(coop) );
	}

template< typename Activity_Tracker >
void
env_infrastructure_t< Activity_Tracker >::ready_to_deregister_notify(
	coop_shptr_t coop ) noexcept
	{
		m_final_dereg_chain.append( std::move(coop) );
	}

template< typename Activity_Tracker >
bool
env_infrastructure_t< Activity_Tracker >::final_deregister_coop(
	coop_shptr_t coop ) noexcept
	{
		return m_coop_repo.final_deregister_coop( std::move(coop) )
				.m_has_live_coop;
	}

template< typename Activity_Tracker >
so_5::timer_id_t
env_infrastructure_t< Activity_Tracker >::schedule_timer(
	const std::type_index & type_wrapper,
	const message_ref_t & msg,
	const mbox_t & mbox,
	std::chrono::steady_clock::duration pause,
	std::chrono::steady_clock::duration period )
	{
		auto timer = m_timer_manager->schedule(
				type_wrapper,
				mbox,
				msg,
				pause,
				period );

		return timer;
	}

template< typename Activity_Tracker >
void
env_infrastructure_t< Activity_Tracker >::single_timer(
	const std::type_index & type_wrapper,
	const message_ref_t & msg,
	const mbox_t & mbox,
	std::chrono::steady_clock::duration pause )
	{
		m_timer_manager->schedule_anonymous(
				type_wrapper,
				mbox,
				msg,
				pause,
				std::chrono::milliseconds::zero() );
	}

template< typename Activity_Tracker >
stats::controller_t &
env_infrastructure_t< Activity_Tracker >::stats_controller() noexcept
	{
		return m_stats_controller;
	}

template< typename Activity_Tracker >
stats::repository_t &
env_infrastructure_t< Activity_Tracker >::stats_repository() noexcept
	{
		return m_stats_controller;
	}

template< typename Activity_Tracker >
so_5::environment_infrastructure_t::coop_repository_stats_t
env_infrastructure_t< Activity_Tracker >::query_coop_repository_stats()
	{
		const auto stats = m_coop_repo.query_stats();

		return environment_infrastructure_t::coop_repository_stats_t{
				stats.m_total_coop_count,
				stats.m_total_agent_count,
				m_final_dereg_chain.size()
		};
	}

template< typename Activity_Tracker >
timer_thread_stats_t
env_infrastructure_t< Activity_Tracker >::query_timer_thread_stats()
	{
		return m_timer_manager->query_stats();
	}

template< typename Activity_Tracker >
disp_binder_shptr_t
env_infrastructure_t< Activity_Tracker >::make_default_disp_binder()
	{
		return { m_default_disp };
	}

template< typename Activity_Tracker >
void
env_infrastructure_t< Activity_Tracker >::run_default_dispatcher_and_go_further(
	env_init_t init_fn )
	{
		::so_5::impl::run_stage(
				"run_default_dispatcher",
				[this] {
					m_default_disp = std::make_shared<
							default_dispatcher_t< Activity_Tracker > >(
									outliving_mutable(m_env),
									outliving_mutable(m_event_queue),
									outliving_mutable(m_activity_tracker) );
				},
				[this] {
					m_default_disp.reset();
				},
				[this, init_fn=std::move(init_fn)] {
					run_user_supplied_init_and_do_main_loop( std::move(init_fn) );
				} );
	}

template< typename Activity_Tracker >
void
env_infrastructure_t< Activity_Tracker >::run_user_supplied_init_and_do_main_loop(
	env_init_t init_fn )
	{
		/*
			If init_fn throws an exception we can found ourselves
			in a situation where there are some working coops.
			Those coops should be correctly deregistered. It means
			that we should usual main loop even in the case of
			an exception from init_fn. But this main loop should
			work only until all coops will be deregistered.

			To do that we will catch an exception from init and
			initiate shutdown even before the call to run_main_loop().
			Then we call run_main_loop() and wail for its completion.
			Then we reraise the exception caught.

			Note that in this scheme run_main_loop() should be
			noexcept function because otherwise we will loose the
			initial exception from init_fn.
		*/
		optional< std::exception_ptr > exception_from_init;
		try
			{
				so_5::impl::wrap_init_fn_call( std::move(init_fn) );
			}
		catch( ... )
			{
				// We can't restore if there will be an exception.
				so_5::details::invoke_noexcept_code( [&] {
						// Store the content of the exception to reraise it later.
						exception_from_init = std::current_exception();
						// Execution should be stopped.
						stop();
					} );
			}

		// We don't expect exceptions from the main loop.
		so_5::details::invoke_noexcept_code( [this] {
				run_main_loop();
			} );

		// If there was an exception from init_fn this exception
		// should be rethrown.
		if( exception_from_init )
			std::rethrow_exception( *exception_from_init );
	}

template< typename Activity_Tracker >
void
env_infrastructure_t< Activity_Tracker >::run_main_loop() noexcept
	{
		// Assume that waiting for new demands is started.
		// This call is necessary because if there is a demand
		// in event queue then m_activity_tracker.wait_stopped() will be
		// called without previous m_activity_tracker.wait_started().
		m_activity_tracker.wait_started();

		for(;;)
			{
				// The first step: all pending final deregs must be processed.
				process_final_deregs_if_any();

				// There can be pending shutdown operation. It must be handled.
				perform_shutdown_related_actions_if_needed();
				if( shutdown_status_t::completed == m_shutdown_status )
					break;

				// The next step: all timers must be converted to events.
				handle_expired_timers_if_any();

				// The last step: an attempt to process demand.
				// Or sleep for some time until next demand arrived.
				try_handle_next_demand();
			}
	}

template< typename Activity_Tracker >
void
env_infrastructure_t< Activity_Tracker >::process_final_deregs_if_any() noexcept
	{
		// This loop is necessary because it is possible that new
		// final dereg demand will be added during processing of
		// the current final dereg demand.
		while( !m_final_dereg_chain.empty() )
			{
				so_5::impl::process_final_dereg_chain(
						m_final_dereg_chain.giveout_current_chain() );
			}
	}

template< typename Activity_Tracker >
void
env_infrastructure_t< Activity_Tracker >::perform_shutdown_related_actions_if_needed() noexcept
	{
		if( shutdown_status_t::must_be_started == m_shutdown_status )
			{
				// Shutdown procedure must be started.
				m_shutdown_status = shutdown_status_t::in_progress;

				// All registered cooperations must be deregistered now.
				m_coop_repo.deregister_all_coop();
			}

		if( shutdown_status_t::in_progress == m_shutdown_status )
			{
				// If there is no more live coops then shutdown must be completed.
				if( !m_coop_repo.has_live_coop() )
					m_shutdown_status = shutdown_status_t::completed;
			}
	}

template< typename Activity_Tracker >
void
env_infrastructure_t< Activity_Tracker >::handle_expired_timers_if_any() noexcept
	{
		// All expired timers must be collected.
		// NOTE: a direct delivery happens in this environment infrastructure.
		m_timer_manager->process_expired_timers();
	}

template< typename Activity_Tracker >
void
env_infrastructure_t< Activity_Tracker >::try_handle_next_demand() noexcept
	{
		execution_demand_t demand;
		const auto pop_result = m_event_queue.pop( demand );
		// If there is no demands we must go to sleep for some time...
		if( event_queue_impl_t::pop_result_t::empty_queue == pop_result )
			{
				// ... but we should go to sleep only if there is no
				// pending final deregistration actions.
				if( m_final_dereg_chain.empty() )
					{
						// We must try to sleep for next timer but only if
						// there is any timer.
						if( !m_timer_manager->empty() )
							{
								// Tracking time for 'waiting' state must be turned on.
								m_activity_tracker.wait_start_if_not_started();

								const auto sleep_time =
										m_timer_manager->timeout_before_nearest_timer(
												// We can use very large value here.
												std::chrono::hours(24) );

								std::this_thread::sleep_for( sleep_time );
							}
						else
							// There are no demands and there are no timers.
							// Environment's work must be finished.
							stop();
					}
			}
		else
			{
				// Tracking time for 'waiting' must be turned off, but
				// tracking time for 'working' must be tuned on and then off again.
				m_activity_tracker.wait_stopped();
				m_activity_tracker.work_started();
				auto work_tracking_stopper = so_5::details::at_scope_exit(
						[this]{ m_activity_tracker.work_stopped(); } );

				// There is at least one demand to process.
				m_default_disp->handle_demand( demand );
			}
	}

//
// ensure_autoshutdown_enabled
//
/*!
 * Throws an exception if autoshutdown feature is disabled.
 *
 * \since v.5.5.19
 */
void
ensure_autoshutdown_enabled(
	const environment_params_t & env_params )
	{
		if( env_params.autoshutdown_disabled() )
			SO_5_THROW_EXCEPTION( rc_autoshutdown_must_be_enabled,
					"autoshutdown feature must be enabled for "
					"so_5::env_infrastructures::simple_not_mtsafe" );
	}

} /* namespace impl */

//
// simple_not_mtsafe_st_env_infrastructure_factory
//
SO_5_FUNC environment_infrastructure_factory_t
factory( params_t && infrastructure_params )
	{
		using namespace impl;

		return [infrastructure_params](
				environment_t & env,
				environment_params_t & env_params,
				mbox_t stats_distribution_mbox )
		{
			ensure_autoshutdown_enabled( env_params );

			environment_infrastructure_t * obj = nullptr;

			const auto & timer_manager_factory =
					infrastructure_params.timer_manager();

			// Create environment infrastructure object in dependence of
			// work thread activity tracking flag.
			const auto tracking = env_params.work_thread_activity_tracking();
			if( work_thread_activity_tracking_t::on == tracking )
				obj = new env_infrastructure_t< reusable::real_activity_tracker_t >(
					env,
					std::move(timer_manager_factory),
					env_params.so5_error_logger(),
					env_params.so5_giveout_coop_listener(),
					std::move(stats_distribution_mbox) );
			else
				obj = new env_infrastructure_t< reusable::fake_activity_tracker_t >(
					env,
					std::move(timer_manager_factory),
					env_params.so5_error_logger(),
					env_params.so5_giveout_coop_listener(),
					std::move(stats_distribution_mbox) );

			return environment_infrastructure_unique_ptr_t(
					obj,
					environment_infrastructure_t::default_deleter() );
		};
	}

} /* namespace simple_not_mtsafe */

} /* namespace env_infrastructures */

} /* namespace so_5 */


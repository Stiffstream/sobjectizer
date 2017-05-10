/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief A very simple not-multithreaded-safe single thread
 * environment infrastructure.
 *
 * \since
 * v.5.5.19
 */

#include <so_5/rt/impl/h/run_stage.hpp>

#include <so_5/disp/reuse/h/data_source_prefix_helpers.hpp>

#include <so_5/rt/h/environment.hpp>
#include <so_5/rt/h/env_infrastructures.hpp>

#include <so_5/details/h/at_scope_exit.hpp>
#include <so_5/details/h/sync_helpers.hpp>

#include <so_5/h/stdcpp.hpp>

#include <so_5/rt/impl/h/st_env_infrastructure_reuse.hpp>

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
 * \since
 * v.5.5.19
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

		virtual void
		push( execution_demand_t demand ) override
			{
				m_demands.push_back( std::move(demand) );
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
		pop( execution_demand_t & receiver ) SO_5_NOEXCEPT
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
 * \since
 * v.5.5.19
 */
using coop_repo_t = reusable::coop_repo_t;

//
// default_disp_impl_basis_t
//
/*!
 * \brief A basic part of implementation of dispatcher interface to be used in
 * places where default dispatcher is needed.
 *
 * \since
 * v.5.5.19
 */
using default_disp_impl_basis_t =
	reusable::default_disp_impl_basis_t< event_queue_impl_t >;

//
// default_disp_binder_t
//
/*!
 * \brief An implementation of disp_binder interface for default dispatcher
 * for this environment infrastructure.
 *
 * \since
 * v.5.5.19
 */
using default_disp_binder_t =
	reusable::default_disp_binder_t< default_disp_impl_basis_t >;

//
// disp_ds_name_parts_t
//
/*!
 * \brief A special class for generation of names for dispatcher data sources.
 *
 * \since
 * v.5.5.19
 */
struct disp_ds_name_parts_t
	{
		static const char * disp_type_part() { return "not_mtsafe_st_env"; }
	};

//
// default_disp_impl_t
//
/*!
 * \brief An implementation of dispatcher interface to be used in
 * places where default dispatcher is needed.
 *
 * \tparam ACTIVITY_TRACKER a type of activity tracker to be used
 * for run-time statistics.
 *
 * \since
 * v.5.5.19
 */
template< typename ACTIVITY_TRACKER >
using default_disp_impl_t =
	reusable::default_disp_impl_t<
			event_queue_impl_t,
			ACTIVITY_TRACKER,
			disp_ds_name_parts_t >;

//
// stats_controller_t
//
/*!
 * \brief Implementation of stats_controller for that type of
 * single-threaded environment.
 *
 * \since
 * v.5.5.19
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
 * \tparam ACTIVITY_TRACKER A type for tracking activity of main working thread.
 *
 * \since
 * v.5.5.19
 */
template< typename ACTIVITY_TRACKER >
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

		virtual void
		launch( env_init_t init_fn ) override;

		virtual void
		stop() override;

		virtual void
		register_coop(
			coop_unique_ptr_t coop ) override;

		virtual void
		deregister_coop(
			nonempty_name_t name,
			coop_dereg_reason_t dereg_reason ) override;

		virtual void
		ready_to_deregister_notify(
			coop_t * coop ) override;

		virtual bool
		final_deregister_coop(
			std::string coop_name ) override;

		virtual so_5::timer_id_t
		schedule_timer(
			const std::type_index & type_wrapper,
			const message_ref_t & msg,
			const mbox_t & mbox,
			std::chrono::steady_clock::duration pause,
			std::chrono::steady_clock::duration period ) override;

		virtual void
		single_timer(
			const std::type_index & type_wrapper,
			const message_ref_t & msg,
			const mbox_t & mbox,
			std::chrono::steady_clock::duration pause ) override;

		virtual stats::controller_t &
		stats_controller() SO_5_NOEXCEPT override;

		virtual stats::repository_t &
		stats_repository() SO_5_NOEXCEPT override;

		virtual dispatcher_t &
		query_default_dispatcher() override;

		virtual so_5::environment_infrastructure_t::coop_repository_stats_t
		query_coop_repository_stats() override;

		virtual timer_thread_stats_t
		query_timer_thread_stats() override;

		virtual disp_binder_unique_ptr_t
		make_default_disp_binder() override;

	private :
		environment_t & m_env;

		//! Type of container for final deregistration demands.
		using final_dereg_coop_container_t = std::deque< coop_t * >;

		//! Queue for final deregistration demands.
		final_dereg_coop_container_t m_final_dereg_coops;

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
		ACTIVITY_TRACKER m_activity_tracker;

		//! Dispatcher to be used as default dispatcher.
		default_disp_impl_t< ACTIVITY_TRACKER > m_default_disp;

		//! Stats controller for this environment.
		stats_controller_t m_stats_controller;

		void
		run_default_dispatcher_and_go_further(
			env_init_t init_fn );

		void
		run_user_supplied_init_and_do_main_loop(
			env_init_t init_fn );

		void
		run_main_loop();

		void
		process_final_deregs_if_any();

		void
		perform_shutdown_related_actions_if_needed();

		void
		handle_expired_timers_if_any();

		void
		try_handle_next_demand();
	};

template< typename ACTIVITY_TRACKER >
env_infrastructure_t< ACTIVITY_TRACKER >::env_infrastructure_t(
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
	,	m_coop_repo( env, std::move(coop_listener) )
	,	m_default_disp(
			outliving_mutable(m_event_queue),
			outliving_mutable(m_activity_tracker) )
	,	m_stats_controller(
			m_env,
			std::move(stats_distribution_mbox),
			stats::impl::st_env_stuff::next_turn_mbox_t::make() )
	{}

template< typename ACTIVITY_TRACKER >
void
env_infrastructure_t< ACTIVITY_TRACKER >::launch( env_init_t init_fn )
	{
		run_default_dispatcher_and_go_further( std::move(init_fn) );
	}

template< typename ACTIVITY_TRACKER >
void
env_infrastructure_t< ACTIVITY_TRACKER >::stop()
	{
		if( shutdown_status_t::not_started == m_shutdown_status )
			{
				m_shutdown_status = shutdown_status_t::must_be_started;
			}
	}

template< typename ACTIVITY_TRACKER >
void
env_infrastructure_t< ACTIVITY_TRACKER >::register_coop(
	coop_unique_ptr_t coop )
	{
		m_coop_repo.register_coop( std::move(coop) );
	}

template< typename ACTIVITY_TRACKER >
void
env_infrastructure_t< ACTIVITY_TRACKER >::deregister_coop(
	nonempty_name_t name,
	coop_dereg_reason_t dereg_reason )
	{
		m_coop_repo.deregister_coop( std::move(name), dereg_reason );
	}

template< typename ACTIVITY_TRACKER >
void
env_infrastructure_t< ACTIVITY_TRACKER >::ready_to_deregister_notify(
	coop_t * coop )
	{
		m_final_dereg_coops.push_back( coop );
	}

template< typename ACTIVITY_TRACKER >
bool
env_infrastructure_t< ACTIVITY_TRACKER >::final_deregister_coop(
	std::string coop_name )
	{
		return m_coop_repo.final_deregister_coop( std::move(coop_name) )
				.m_has_live_coop;
	}

template< typename ACTIVITY_TRACKER >
so_5::timer_id_t
env_infrastructure_t< ACTIVITY_TRACKER >::schedule_timer(
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

template< typename ACTIVITY_TRACKER >
void
env_infrastructure_t< ACTIVITY_TRACKER >::single_timer(
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

template< typename ACTIVITY_TRACKER >
stats::controller_t &
env_infrastructure_t< ACTIVITY_TRACKER >::stats_controller() SO_5_NOEXCEPT
	{
		return m_stats_controller;
	}

template< typename ACTIVITY_TRACKER >
stats::repository_t &
env_infrastructure_t< ACTIVITY_TRACKER >::stats_repository() SO_5_NOEXCEPT
	{
		return m_stats_controller;
	}

template< typename ACTIVITY_TRACKER >
dispatcher_t &
env_infrastructure_t< ACTIVITY_TRACKER >::query_default_dispatcher()
	{
		return m_default_disp;
	}

template< typename ACTIVITY_TRACKER >
so_5::environment_infrastructure_t::coop_repository_stats_t
env_infrastructure_t< ACTIVITY_TRACKER >::query_coop_repository_stats()
	{
		const auto stats = m_coop_repo.query_stats();

		return environment_infrastructure_t::coop_repository_stats_t{
				stats.m_registered_coop_count,
				stats.m_deregistered_coop_count,
				stats.m_total_agent_count,
				m_final_dereg_coops.size()
		};
	}

template< typename ACTIVITY_TRACKER >
timer_thread_stats_t
env_infrastructure_t< ACTIVITY_TRACKER >::query_timer_thread_stats()
	{
		return m_timer_manager->query_stats();
	}

template< typename ACTIVITY_TRACKER >
disp_binder_unique_ptr_t
env_infrastructure_t< ACTIVITY_TRACKER >::make_default_disp_binder()
	{
		return stdcpp::make_unique< default_disp_binder_t >(
				outliving_mutable(m_default_disp) );
	}

template< typename ACTIVITY_TRACKER >
void
env_infrastructure_t< ACTIVITY_TRACKER >::run_default_dispatcher_and_go_further(
	env_init_t init_fn )
	{
		::so_5::impl::run_stage(
				"run_default_dispatcher",
				[this] {
					m_default_disp.set_data_sources_name_base( "DEFAULT" );
					m_default_disp.start( m_env );
				},
				[this] {
					m_default_disp.shutdown();
					m_default_disp.wait();
				},
				[this, init_fn] {
					run_user_supplied_init_and_do_main_loop( std::move(init_fn) );
				} );
	}

template< typename ACTIVITY_TRACKER >
void
env_infrastructure_t< ACTIVITY_TRACKER >::run_user_supplied_init_and_do_main_loop(
	env_init_t init_fn )
	{
		init_fn();
		run_main_loop();
	}

template< typename ACTIVITY_TRACKER >
void
env_infrastructure_t< ACTIVITY_TRACKER >::run_main_loop()
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

template< typename ACTIVITY_TRACKER >
void
env_infrastructure_t< ACTIVITY_TRACKER >::process_final_deregs_if_any()
	{
		// This loop is necessary because it is possible that new
		// final dereg demand will be added during processing of
		// the current final dereg demand.
		while( !m_final_dereg_coops.empty() )
			{
				final_dereg_coop_container_t coops;
				coops.swap( m_final_dereg_coops );

				for( auto ptr : coops )
					coop_t::call_final_deregister_coop( ptr );
			}
	}

template< typename ACTIVITY_TRACKER >
void
env_infrastructure_t< ACTIVITY_TRACKER >::perform_shutdown_related_actions_if_needed()
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

template< typename ACTIVITY_TRACKER >
void
env_infrastructure_t< ACTIVITY_TRACKER >::handle_expired_timers_if_any()
	{
		// All expired timers must be collected.
		// NOTE: a direct delivery happens in this environment infrastructure.
		m_timer_manager->process_expired_timers();
	}

template< typename ACTIVITY_TRACKER >
void
env_infrastructure_t< ACTIVITY_TRACKER >::try_handle_next_demand()
	{
		execution_demand_t demand;
		const auto pop_result = m_event_queue.pop( demand );
		// If there is no demands we must go to sleep for some time...
		if( event_queue_impl_t::pop_result_t::empty_queue == pop_result )
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
		else
			{
				// Tracking time for 'waiting' must be turned off, but
				// tracking time for 'working' must be tuned on and then off again.
				m_activity_tracker.wait_stopped();
				m_activity_tracker.work_started();
				auto work_tracking_stopper = so_5::details::at_scope_exit(
						[this]{ m_activity_tracker.work_stopped(); } );

				// There is at least one demand to process.
				m_default_disp.handle_demand( demand );
			}
	}

//
// ensure_autoshutdown_enabled
//
/*!
 * Throws an exception if autoshutdown feature is disabled.
 *
 * \since
 * v.5.5.19
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
					env_params.so5__error_logger(),
					env_params.so5__giveout_coop_listener(),
					std::move(stats_distribution_mbox) );
			else
				obj = new env_infrastructure_t< reusable::fake_activity_tracker_t >(
					env,
					std::move(timer_manager_factory),
					env_params.so5__error_logger(),
					env_params.so5__giveout_coop_listener(),
					std::move(stats_distribution_mbox) );

			return environment_infrastructure_unique_ptr_t(
					obj,
					environment_infrastructure_t::default_deleter() );
		};
	}

} /* namespace simple_not_mtsafe */

} /* namespace env_infrastructures */

} /* namespace so_5 */


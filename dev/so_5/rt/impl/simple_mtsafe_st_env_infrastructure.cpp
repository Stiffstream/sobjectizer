/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief A very simple multithreaded-safe single thread
 * environment infrastructure.
 *
 * \since
 * v.5.5.19
 */

#include <so_5/rt/impl/h/run_stage.hpp>

#include <so_5/disp/reuse/h/data_source_prefix_helpers.hpp>

#include <so_5/rt/environment.hpp>
#include <so_5/rt/send_functions.hpp>

#include <so_5/details/h/at_scope_exit.hpp>
#include <so_5/details/h/sync_helpers.hpp>

#include <so_5/stdcpp.hpp>

#include <so_5/rt/impl/h/st_env_infrastructure_reuse.hpp>

namespace so_5 {

namespace env_infrastructures {

namespace simple_mtsafe {

namespace impl {

namespace helpers {

template< typename Action >
auto
unlock_do_and_lock_again(
	std::unique_lock< std::mutex > & acquired_lock,
	Action && action ) -> decltype(action())
	{
		acquired_lock.unlock();
		auto relock_again = so_5::details::at_scope_exit( [&acquired_lock]{
				acquired_lock.lock();
			} );

		return action();
	}

} /* namespace helpers */

//! A short name for namespace with run-time stats stuff.
namespace stats = ::so_5::stats;

//! A short name for namespace with reusable stuff.
namespace reusable = ::so_5::env_infrastructures::st_reusable_stuff;

/*!
 * \brief Status of the main thread on which environment is working.
 *
 * Main thread can handle some events or can sleep and wait for new events.
 *
 * \since
 * v.5.5.19
 */
enum class main_thread_status_t
	{
		working,
		waiting
	};

/*!
 * \brief A bunch of sync objects which need to be shared between
 * various parts of env_infrastructure.
 *
 * \since
 * v.5.5.19
 */
struct main_thread_sync_objects_t
	{
		//! Main lock for environment infrastructure.
		std::mutex m_lock;
		//! A condition to sleep on when no activities to handle.
		std::condition_variable m_wakeup_condition;

		//! The current status of the main thread.
		main_thread_status_t m_status{ main_thread_status_t::working };
	};

/*!
 * \note
 * Mutex from sync_objects must be already acquired!
 */
inline void
wakeup_if_waiting( main_thread_sync_objects_t & sync_objects )
	{
		if( main_thread_status_t::waiting == sync_objects.m_status )
			sync_objects.m_wakeup_condition.notify_one();
	}

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

		event_queue_impl_t( main_thread_sync_objects_t & sync_objects )
			:	m_sync_objects( sync_objects )
			{}

		/*!
		 * \note
		 * This method locks the main mutex by itself.
		 */
		virtual void
		push( execution_demand_t demand ) override
			{
				std::lock_guard< std::mutex > lock( m_sync_objects.m_lock );

				m_demands.push_back( std::move(demand) );

				wakeup_if_waiting( m_sync_objects );
			}

		/*!
		 * \note
		 * This method locks the main mutex by itself.
		 */
		stats_t
		query_stats() const
			{
				std::lock_guard< std::mutex > lock( m_sync_objects.m_lock );

				return { m_demands.size() };
			}

		//! Type for result of extraction operation.
		enum class pop_result_t
			{
				extracted,
				empty_queue
			};

		/*!
		 * \note
		 * NOTE: this method must be called only when main thread's mutex
		 * is locked.
		 */
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
		main_thread_sync_objects_t & m_sync_objects;

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
		static const char * disp_type_part() { return "mtsafe_st_env"; }
	};

//
// default_disp_impl_t
//
/*!
 * \brief An implementation of dispatcher interface to be used in
 * places where default dispatcher is needed.
 *
 * \tparam Activity_Tracker a type of activity tracker to be used
 * for run-time statistics.
 *
 * \since
 * v.5.5.19
 */
template< typename Activity_Tracker >
using default_disp_impl_t =
	reusable::default_disp_impl_t<
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
 * \since
 * v.5.5.19
 */
using stats_controller_t =
	reusable::stats_controller_t< so_5::details::actual_lock_holder_t<> >;

//
// env_infrastructure_t
//
/*!
 * \brief Default implementation of multithreaded environment infrastructure.
 *
 * \tparam Activity_Tracker A type for tracking activity of main working thread.
 *
 * \since
 * v.5.5.19
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

		//! All sync objects to be shared between different parts.
		main_thread_sync_objects_t m_sync_objects;

		//! Type of container for final deregistration demands.
		using final_dereg_coop_container_t = std::deque< coop_t * >;

		//! Queue for final deregistration demands.
		final_dereg_coop_container_t m_final_dereg_coops;

		//! Status of shutdown procedure.
		shutdown_status_t m_shutdown_status{ shutdown_status_t::not_started };

		//! A collector for elapsed timers.
		reusable::actual_elapsed_timers_collector_t m_timers_collector;

		//! A timer manager to be used.
		timer_manager_unique_ptr_t m_timer_manager;

		//! Queue for execution_demands which must be handled on the main thread.
		event_queue_impl_t m_event_queue;

		//! Repository of registered coops.
		coop_repo_t m_coop_repo;

		//! Actual activity tracker for main working thread.
		Activity_Tracker m_activity_tracker;

		//! Dispatcher to be used as default dispatcher.
		default_disp_impl_t< Activity_Tracker > m_default_disp;

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
		process_final_deregs_if_any(
			std::unique_lock< std::mutex > & acquired_lock );

		void
		perform_shutdown_related_actions_if_needed(
			std::unique_lock< std::mutex > & acquired_lock );

		void
		handle_expired_timers_if_any(
			std::unique_lock< std::mutex > & acquired_lock );

		void
		try_handle_next_demand(
			std::unique_lock< std::mutex > & acquired_lock );
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
	,	m_event_queue( m_sync_objects )
	,	m_coop_repo( env, std::move(coop_listener) )
	,	m_default_disp(
			outliving_mutable(m_event_queue),
			outliving_mutable(m_activity_tracker) )
	,	m_stats_controller(
			m_env,
			std::move(stats_distribution_mbox),
			stats::impl::st_env_stuff::next_turn_mbox_t::make() )
	{}

template< typename Activity_Tracker >
void
env_infrastructure_t< Activity_Tracker >::launch( env_init_t init_fn )
	{
		run_default_dispatcher_and_go_further( std::move(init_fn) );
	}

template< typename Activity_Tracker >
void
env_infrastructure_t< Activity_Tracker >::stop()
	{
		std::lock_guard< std::mutex > lock( m_sync_objects.m_lock );

		if( shutdown_status_t::not_started == m_shutdown_status )
			{
				m_shutdown_status = shutdown_status_t::must_be_started;
				wakeup_if_waiting( m_sync_objects );
			}
	}

template< typename Activity_Tracker >
void
env_infrastructure_t< Activity_Tracker >::register_coop(
	coop_unique_ptr_t coop )
	{
		m_coop_repo.register_coop( std::move(coop) );
	}

template< typename Activity_Tracker >
void
env_infrastructure_t< Activity_Tracker >::deregister_coop(
	nonempty_name_t name,
	coop_dereg_reason_t dereg_reason )
	{
		m_coop_repo.deregister_coop( std::move(name), dereg_reason );
	}

template< typename Activity_Tracker >
void
env_infrastructure_t< Activity_Tracker >::ready_to_deregister_notify(
	coop_t * coop )
	{
		std::lock_guard< std::mutex > lock( m_sync_objects.m_lock );
		m_final_dereg_coops.push_back( coop );

		wakeup_if_waiting( m_sync_objects );
	}

template< typename Activity_Tracker >
bool
env_infrastructure_t< Activity_Tracker >::final_deregister_coop(
	std::string coop_name )
	{
		return m_coop_repo.final_deregister_coop( std::move(coop_name) )
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
		std::lock_guard< std::mutex > lock( m_sync_objects.m_lock );

		auto timer = m_timer_manager->schedule(
				type_wrapper,
				mbox,
				msg,
				pause,
				period );

		wakeup_if_waiting( m_sync_objects );

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
		std::lock_guard< std::mutex > lock( m_sync_objects.m_lock );

		m_timer_manager->schedule_anonymous(
				type_wrapper,
				mbox,
				msg,
				pause,
				std::chrono::milliseconds::zero() );

		wakeup_if_waiting( m_sync_objects );
	}

template< typename Activity_Tracker >
stats::controller_t &
env_infrastructure_t< Activity_Tracker >::stats_controller() SO_5_NOEXCEPT
	{
		return m_stats_controller;
	}

template< typename Activity_Tracker >
stats::repository_t &
env_infrastructure_t< Activity_Tracker >::stats_repository() SO_5_NOEXCEPT
	{
		return m_stats_controller;
	}

template< typename Activity_Tracker >
dispatcher_t &
env_infrastructure_t< Activity_Tracker >::query_default_dispatcher()
	{
		return m_default_disp;
	}

template< typename Activity_Tracker >
so_5::environment_infrastructure_t::coop_repository_stats_t
env_infrastructure_t< Activity_Tracker >::query_coop_repository_stats()
	{
		std::lock_guard< std::mutex > lock( m_sync_objects.m_lock );

		const auto stats = m_coop_repo.query_stats();

		return environment_infrastructure_t::coop_repository_stats_t{
				stats.m_registered_coop_count,
				stats.m_deregistered_coop_count,
				stats.m_total_agent_count,
				m_final_dereg_coops.size()
		};
	}

template< typename Activity_Tracker >
timer_thread_stats_t
env_infrastructure_t< Activity_Tracker >::query_timer_thread_stats()
	{
		std::lock_guard< std::mutex > lock( m_sync_objects.m_lock );

		return m_timer_manager->query_stats();
	}

template< typename Activity_Tracker >
disp_binder_unique_ptr_t
env_infrastructure_t< Activity_Tracker >::make_default_disp_binder()
	{
		return stdcpp::make_unique< default_disp_binder_t >(
				outliving_mutable(m_default_disp) );
	}

template< typename Activity_Tracker >
void
env_infrastructure_t< Activity_Tracker >::run_default_dispatcher_and_go_further(
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

template< typename Activity_Tracker >
void
env_infrastructure_t< Activity_Tracker >::run_user_supplied_init_and_do_main_loop(
	env_init_t init_fn )
	{
		init_fn();
		run_main_loop();
	}

template< typename Activity_Tracker >
void
env_infrastructure_t< Activity_Tracker >::run_main_loop()
	{
		// Assume that waiting for new demands is started.
		// This call is necessary because if there is a demand
		// in event queue then m_activity_tracker.wait_stopped() will be
		// called without previous m_activity_tracker.wait_started().
		m_activity_tracker.wait_started();

		// Acquire the main lock for the first time.
		// It will be released and reacquired many times later.
		std::unique_lock< std::mutex > lock( m_sync_objects.m_lock );
		for(;;)
			{
				// The first step: all pending final deregs must be processed.
				process_final_deregs_if_any( lock );

				// There can be pending shutdown operation. It must be handled.
				perform_shutdown_related_actions_if_needed( lock );
				if( shutdown_status_t::completed == m_shutdown_status )
					break;

				// The next step: all timers must be converted to events.
				handle_expired_timers_if_any( lock );

				// The last step: an attempt to process demand.
				// Or sleep for some time until next demand arrived.
				try_handle_next_demand( lock );
			}
	}

template< typename Activity_Tracker >
void
env_infrastructure_t< Activity_Tracker >::process_final_deregs_if_any(
	std::unique_lock< std::mutex > & acquired_lock )
	{
		// This loop is necessary because it is possible that new
		// final dereg demand will be added during processing of
		// the current final dereg demand.
		while( !m_final_dereg_coops.empty() )
			{
				final_dereg_coop_container_t coops;
				coops.swap( m_final_dereg_coops );

				helpers::unlock_do_and_lock_again( acquired_lock,
					[&coops] {
						for( auto ptr : coops )
							coop_t::call_final_deregister_coop( ptr );
					} );
			}
	}

template< typename Activity_Tracker >
void
env_infrastructure_t< Activity_Tracker >::perform_shutdown_related_actions_if_needed(
	std::unique_lock< std::mutex > & acquired_lock )
	{
		if( shutdown_status_t::must_be_started == m_shutdown_status )
			{
				// Shutdown procedure must be started.
				m_shutdown_status = shutdown_status_t::in_progress;

				// All registered cooperations must be deregistered now.
				// We must unlock out main lock because there is a need to
				// push a final event for deregisteing agents to event queue.
				helpers::unlock_do_and_lock_again( acquired_lock,
					[this] {
						m_coop_repo.deregister_all_coop();
					} );
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
env_infrastructure_t< Activity_Tracker >::handle_expired_timers_if_any(
	std::unique_lock< std::mutex > & acquired_lock )
	{
		// All expired timers must be collected.
		m_timer_manager->process_expired_timers();

		if( !m_timers_collector.empty() )
			// Actual handling of elapsed timers must be done
			// on unlocked env_infrastructure.
			// It is necessary because there can be an attempt to
			// deliver delayed/periodic messages into event_queue of
			// the default_dispatcher. But it is impossible to do if
			// env_infrastructure's mutex is locked.
			helpers::unlock_do_and_lock_again( acquired_lock,
				[this] {
					m_timers_collector.process();
				} );
	}

template< typename Activity_Tracker >
void
env_infrastructure_t< Activity_Tracker >::try_handle_next_demand(
	std::unique_lock< std::mutex > & acquired_lock )
	{
		execution_demand_t demand;
		const auto pop_result = m_event_queue.pop( demand );
		// If there is no demands we must go to sleep for some time...
		if( event_queue_impl_t::pop_result_t::empty_queue == pop_result )
			{
				// Tracking time for 'waiting' state must be turned on.
				m_activity_tracker.wait_start_if_not_started();

				const auto sleep_time =
						m_timer_manager->timeout_before_nearest_timer(
								std::chrono::minutes(1) );

				m_sync_objects.m_status = main_thread_status_t::waiting;

				m_sync_objects.m_wakeup_condition.wait_for(
						acquired_lock,
						sleep_time );

				m_sync_objects.m_status = main_thread_status_t::working;
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
				helpers::unlock_do_and_lock_again( acquired_lock,
					[this, &demand] {
						m_default_disp.handle_demand( demand );
					} );
			}
	}

} /* namespace impl */

//
// simple_mtsafe_st_env_infrastructure_factory
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

} /* namespace simple_mtsafe */

} /* namespace env_infrastructures */

} /* namespace so_5 */


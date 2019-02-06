/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Reusable stuff to be used in implementations of
 * various single thread environment infrastructure.
 *
 * \since
 * v.5.5.19
 */

#pragma once

#include <so_5/environment_infrastructure.hpp>

#include <so_5/impl/coop_repository_basis.hpp>
#include <so_5/impl/mbox_iface_for_timers.hpp>

#include <so_5/disp/reuse/data_source_prefix_helpers.hpp>

#include <so_5/stats/impl/activity_tracking.hpp>
#include <so_5/stats/impl/st_env_stuff.hpp>
#include <so_5/stats/controller.hpp>
#include <so_5/stats/repository.hpp>
#include <so_5/stats/prefix.hpp>
#include <so_5/stats/messages.hpp>
#include <so_5/stats/std_names.hpp>

#include <so_5/send_functions.hpp>
#include <so_5/env_infrastructures.hpp>

#include <so_5/timers.hpp>

#include <deque>

namespace so_5 {

namespace env_infrastructures {

namespace st_reusable_stuff {

//! A short name for namespace with run-time stats stuff.
namespace stats = ::so_5::stats;

/*!
 * \brief Status of shutdown procedure.
 *
 * \since
 * v.5.5.19
 */
enum class shutdown_status_t
	{
		//! Shutdown is not started yet.
		not_started,
		//! Shutdown must be started as soon as possible.
		must_be_started,
		//! Shutdown is initiated but not finished yet.
		in_progress,
		//! Shutdown completed and work of environment must be finished.
		completed
	};

//
// fake_activity_tracker_t
//
/*!
 * \brief An implementation of work thread activity tracking stuff for
 * the case when activity tracking is not used.
 *
 * \since
 * v.5.5.19
 */
struct fake_activity_tracker_t final
	{
		void wait_started() {}
		void wait_start_if_not_started() {}
		void wait_stopped() {}

		void work_started() {}
		void work_stopped() {}
	};

//
// real_activity_tracker_t
//
/*!
 * \brief An implementation of work thread activity tracking stuff for
 * the case when activity tracking is used.
 *
 * \attention
 * There is no need to use some sync object (like mutex or spin_lock) because
 * all actions with activity tracker will be done on the context of
 * single thread.
 *
 * \since
 * v.5.5.19
 */
class real_activity_tracker_t final
	{
		stats::activity_tracking_stuff::stats_collector_t<
						stats::activity_tracking_stuff::null_lock >
				m_waiting{};

		stats::activity_tracking_stuff::stats_collector_t<
						stats::activity_tracking_stuff::null_lock >
				m_working{};

	public :
		void wait_started() { m_waiting.start(); }
		void wait_start_if_not_started() { m_waiting.start_if_not_started(); }
		void wait_stopped() { m_waiting.stop(); }

		void work_started() { m_working.start(); }
		void work_stopped() { m_working.stop(); }

		stats::work_thread_activity_stats_t
		take_activity_stats()
			{
				stats::work_thread_activity_stats_t result;

				result.m_working_stats = m_working.take_stats();
				result.m_waiting_stats = m_waiting.take_stats();

				return result;
			}
	};

inline void
send_thread_activity_stats(
	const mbox_t &,
	const stats::prefix_t &,
	const current_thread_id_t &,
	fake_activity_tracker_t & )
	{
		/* Nothing to do */
	}

inline void
send_thread_activity_stats(
	const so_5::mbox_t & mbox,
	const stats::prefix_t & prefix,
	const current_thread_id_t & thread_id,
	real_activity_tracker_t & activity_tracker )
	{
		so_5::send< stats::messages::work_thread_activity >(
				mbox,
				prefix,
				stats::suffixes::work_thread_activity(),
				thread_id,
				activity_tracker.take_activity_stats() );
	}

//
// coop_repo_t
//
/*!
 * \brief Implementation of coop_repository for
 * single-threaded environment infrastructure.
 *
 * \since
 * v.5.5.19
 */
class coop_repo_t final
	:	public ::so_5::impl::coop_repository_basis_t
	{
	public :
		//! Initializing constructor.
		coop_repo_t(
			//! SObjectizer Environment.
			environment_t & env,
			//! Cooperation action listener.
			coop_listener_unique_ptr_t coop_listener )
			:	coop_repository_basis_t( env, std::move(coop_listener) )
			{}

		//! Is there any live coop?
		bool
		has_live_coop()
			{
				// A lock is necessary here because coop_repo can be used
				// in thread-safe environment where access to environment from
				// different thread is allowed.
				std::lock_guard< std::mutex > l{ lock() };
				return !( m_registered_coop.empty() && m_deregistered_coop.empty() );
			}
	};

//
// default_disp_impl_basis_t
//
/*!
 * \brief A basic part of implementation of dispatcher interface to be used in
 * places where default dispatcher is needed.
 *
 * \note
 * This part is not dependent of activity tracking policy which can be used in
 * derived classes.
 *
 * \tparam Event_Queue_Type An actual type of event queue.
 *
 * \since
 * v.5.5.19
 */
template< typename Event_Queue_Type >
class default_disp_impl_basis_t : public dispatcher_t
	{
	public :
		default_disp_impl_basis_t(
			outliving_reference_t< Event_Queue_Type > event_queue )
			:	m_event_queue( std::move(event_queue) )
			{}

		void
		handle_demand(
			execution_demand_t & demand )
			{
				demand.call_handler( m_thread_id );
			}

		Event_Queue_Type &
		event_queue() const noexcept
			{
				return m_event_queue.get();
			}

		current_thread_id_t
		thread_id() const noexcept
			{
				return m_thread_id;
			}

		void
		agent_bound() noexcept
			{
				++m_agents_bound;
			}

		void
		agent_unbound() noexcept
			{
				--m_agents_bound;
			}

		std::size_t
		agents_bound() const noexcept
			{
				return m_agents_bound.load( std::memory_order_relaxed );
			}

	protected :
		//! Event queue for that dispatcher.
		outliving_reference_t< Event_Queue_Type > m_event_queue;

		//! ID of the main thread.
		/*!
		 * Will be passed to event handlers which are called on the context
		 * of the main thread.
		 *
		 * \note
		 * Receives the value only in start() method.
		 */
		current_thread_id_t m_thread_id;

		//! Counter of agents bound to that dispatcher.
		std::atomic< std::size_t > m_agents_bound{ 0 };
	};

//
// default_disp_binder_t
//
/*!
 * \brief An implementation of disp_binder interface for default dispatcher
 * for this environment infrastructure.
 *
 * \tparam Disp_Iface An actual dispatcher interface type.
 * Expected to be default_disp_impl_basis_t<EQ>.
 *
 * \since
 * v.5.5.19
 */
template< typename Disp_Iface >
class default_disp_binder_t final : public so_5::disp_binder_t
	{
	public :
		default_disp_binder_t(
			outliving_reference_t< Disp_Iface > actual_disp )
			:	m_actual_disp( std::move(actual_disp) )
			{}

		virtual disp_binding_activator_t
		bind_agent(
			environment_t & /*env*/,
			agent_ref_t agent ) override
			{
				auto result = [agent, this]() {
					agent->so_bind_to_dispatcher( m_actual_disp.get().event_queue() );
				};

				// Dispatcher must know about yet another agent bound.
				m_actual_disp.get().agent_bound();

				return result;
			}

		virtual void
		unbind_agent(
			environment_t & /*env*/,
			agent_ref_t /*agent*/ ) override
			{
				// Dispatcher must know about yet another agent bound.
				m_actual_disp.get().agent_unbound();
			}

	private :
		//! Actual default dispatcher implementation.
		outliving_reference_t< Disp_Iface > m_actual_disp;
	};

//
// default_disp_impl_t
//
/*!
 * \brief An implementation of dispatcher interface to be used in
 * places where default dispatcher is needed.
 *
 * \tparam Event_Queue_Type a type of actual event queue for the dispatcher.
 *
 * \tparam Activity_Tracker a type of activity tracker to be used
 * for run-time statistics.
 *
 * \tparam Data_Source_Name_Parts a type with methods for generation
 * of names for dispatcher's data sources. Must be a type like that:
 * \code
   struct disp_ds_name_parts_t
		{
			static const char * disp_type_part() { return "mtsafe_st_env"; }
		};
 * \endcode
 *
 * \since
 * v.5.5.19
 */
template<
	typename Event_Queue_Type,
	typename Activity_Tracker,
	typename Data_Source_Name_Parts >
class default_disp_impl_t final
	: public default_disp_impl_basis_t< Event_Queue_Type >
	{
	public :
		default_disp_impl_t(
			outliving_reference_t< Event_Queue_Type > event_queue,
			outliving_reference_t< Activity_Tracker > activity_tracker )
			:	default_disp_impl_basis_t< Event_Queue_Type >( std::move(event_queue) )
			,	m_data_source( outliving_mutable( *this ) )
			,	m_activity_tracker( std::move(activity_tracker) )
			{}

		virtual void
		start(
			environment_t & env ) override
			{
				this->m_thread_id = query_current_thread_id();
				m_data_source.start( outliving_mutable(env.stats_repository()) );
			}

		virtual void
		shutdown() override
			{
				m_data_source.stop();
			}

		virtual void
		wait() override
			{
				// Nothing to do.
			}

		virtual void
		set_data_sources_name_base(
			const std::string & name ) override
			{
				m_data_source.set_data_sources_name_base( name );
			}

		Activity_Tracker &
		activity_tracker() noexcept
			{
				return m_activity_tracker;
			}

	private :

		/*!
		 * \brief Data source for run-time monitoring of whole dispatcher.
		 *
		 * \since
		 * v.5.5.19
		 */
		class disp_data_source_t final : public stats::manually_registered_source_t
			{
				//! Dispatcher to work with.
				outliving_reference_t< default_disp_impl_t > m_dispatcher;

				//! Basic prefix for data sources.
				stats::prefix_t m_base_prefix;

			public :
				disp_data_source_t(
					outliving_reference_t< default_disp_impl_t > disp )
					:	m_dispatcher( std::move(disp) )
					{}

				void
				distribute( const mbox_t & mbox ) override
					{
						so_5::send< stats::messages::quantity< std::size_t > >(
								mbox,
								m_base_prefix,
								stats::suffixes::agent_count(),
								m_dispatcher.get().agents_bound() );

						const auto evt_queue_stats =
								m_dispatcher.get().event_queue().query_stats();
						so_5::send< stats::messages::quantity< std::size_t > >(
								mbox,
								m_base_prefix,
								stats::suffixes::work_thread_queue_size(),
								evt_queue_stats.m_demands_count );

						send_thread_activity_stats(
								mbox,
								m_base_prefix,
								m_dispatcher.get().thread_id(),
								m_dispatcher.get().activity_tracker() );
					}

				void
				set_data_sources_name_base(
					const std::string & name_base )
					{
						using namespace so_5::disp::reuse;

						m_base_prefix = make_disp_prefix(
								Data_Source_Name_Parts::disp_type_part(),
								name_base,
								&m_dispatcher.get() );
					}
			};

		//! Data source for speading run-time stats.
		disp_data_source_t m_data_source;

		//! Activity tracker.
		outliving_reference_t< Activity_Tracker > m_activity_tracker;
	};


//
// actual_elapsed_timers_collector_t
//
/*!
 * \brief Implementation of elapsed_timers_collector interface.
 *
 * \since
 * v.5.5.19
 */
class actual_elapsed_timers_collector_t final
	:	public timer_manager_t::elapsed_timers_collector_t
	{
		//! Type of demand created from elapsed timer.
		struct demand_t
			{
				std::type_index m_type_index;
				mbox_t m_mbox;
				message_ref_t m_message;

				demand_t(
					std::type_index type_index,
					mbox_t mbox,
					message_ref_t message )
					:	m_type_index( std::move(type_index) )
					,	m_mbox( std::move(mbox) )
					,	m_message( std::move(message) )
					{}
			};

		//! Type of container for demands.
		using demands_container_t = std::deque< demand_t >;

		//! Collected demands.
		demands_container_t m_demands;

	public :
		virtual void
		accept(
			std::type_index type_index,
			mbox_t mbox,
			message_ref_t msg ) override
			{
				m_demands.emplace_back(
						std::move(type_index),
						std::move(mbox),
						std::move(msg) );
			}

		/*!
		 * \return true if there is no any pending demands.
		 */
		bool
		empty() const noexcept
			{
				return m_demands.empty();
			}

		/*!
		 * Convert all demands into actual message sends.
		 */
		void
		process()
			{
				for( auto & d : m_demands )
				{
					so_5::rt::impl::mbox_iface_for_timers_t{ d.m_mbox }
							.deliver_message_from_timer( d.m_type_index, d.m_message );
				}

				// A defense from cases where were too many timers.
				if( m_demands.size() < 1000 )
					// A simple clean is enough.
					m_demands.clear();
				else
					{
						// Old container must be utilized.
						demands_container_t demands;
						demands.swap( m_demands );
					}
			}
	};

//
// direct_delivery_elapsed_timers_collector_t
//
/*!
 * \brief Implementation of elapsed_timers_collector interface which
 * is not multi-thread safe.
 *
 * This implementation is intended to use in not-mtsafe environment
 * infrastructures where is possible to transform elapsed timer into
 * a demand directly.
 *
 * It means that actual delivery is performed in accept() method.
 *
 * \since
 * v.5.5.19
 */
class direct_delivery_elapsed_timers_collector_t final
	:	public timer_manager_t::elapsed_timers_collector_t
	{
	public :
		virtual void
		accept(
			std::type_index type_index,
			mbox_t mbox,
			message_ref_t msg ) override
			{
				so_5::rt::impl::mbox_iface_for_timers_t{ mbox }
						.deliver_message_from_timer( type_index, msg );
			}
	};

//
// stats_controller_t
//
/*!
 * \brief Implementation of stats_controller for that type of
 * single-threaded environment.
 *
 * \tparam Lock_Holder a type for defense from multi-threaded access.
 * Expected to be so_5::details::actual_lock_holder_t or
 * so_5::details::no_lock_holder_t.
 *
 * \since
 * v.5.5.19
 */
template< typename Lock_Holder >
class stats_controller_t final
	:	public stats::controller_t
	,	public stats::repository_t
	,	public stats::impl::st_env_stuff::next_turn_handler_t
	,	protected Lock_Holder
	{
	public :
		//! Initializing constructor.
		stats_controller_t(
			//! Environment to work in.
			environment_t & env,
			//! Mbox for sending messages with run-time statistics.
			mbox_t distribution_mbox,
			//! Mbox for delayed messages for initiation of next turn.
			mbox_t next_turn_mbox )
			:	m_env( env )
			,	m_distribution_mbox( std::move(distribution_mbox) )
			,	m_next_turn_mbox( std::move(next_turn_mbox) )
			{}

		// Implementation of controller_t interface.
		virtual const mbox_t &
		mbox() const override
			{
				return m_distribution_mbox;
			}

		virtual void
		turn_on() override
			{
				this->lock_and_perform( [&]{
					if( status_t::off == m_status )
						{
							// Do the part where exceptions are possible.
							const auto run_id = m_run_id + 1;

							send_next_message( very_small_timeout(), run_id );

							// We don't expect exceptions from here.
							m_status = status_t::on;
							m_run_id = run_id;
						}
					} );
			}

		virtual void
		turn_off() override
			{
				this->lock_and_perform( [&] {
					m_status = status_t::off;
				} );
			}

		virtual std::chrono::steady_clock::duration
		set_distribution_period(
			std::chrono::steady_clock::duration period ) override
			{
				return this->lock_and_perform( [&] {
					auto ret_value = m_distribution_period;

					m_distribution_period = period;

					return ret_value;
				} );
			}

		// Implementation of repository_t interface.
		virtual void
		add( stats::source_t & what ) override
			{
				this->lock_and_perform( [&] {
					source_list_add( what, m_head, m_tail );
				} );
			}

		virtual void
		remove( stats::source_t & what ) override
			{
				this->lock_and_perform( [&] {
					source_list_remove( what, m_head, m_tail );
				} );
			}

		// Implementation of next_turn_handler_t interface.
		virtual void
		on_next_turn( int run_id ) override
			{
				this->lock_and_perform( [&] {
					if( status_t::on == m_status && run_id == m_run_id )
						{
							const auto actual_duration = distribute_current_data();

							if( actual_duration < m_distribution_period )
								// There is some time to sleep.
								send_next_message(
										m_distribution_period - actual_duration,
										m_run_id );
							else
								// We must distribute next portion of data
								// without long waiting.
								send_next_message( very_small_timeout(), m_run_id );
						}
				} );
			}

	private :
		//! Status of stats_controller.
		enum class status_t
			{
				off,
				on
			};

		//! Environment to work in.
		/*!
		 * This reference is necessary for sending delayed messages.
		 */
		environment_t & m_env;

		//! Mbox for sending messages with run-time statistics.
		const mbox_t m_distribution_mbox;
		//! Mbox for delayed messages for initiation of next turn.
		const mbox_t m_next_turn_mbox;

		//! Object's lock.
		/*!
		 * In single-threaded invironment we need to have only one lock.
		 */
		std::mutex m_lock;

		//! Current status of stats_controller.
		status_t m_status{ status_t::off };

		//! ID of stats distribution.
		/*!
		 * Will be increased on every turn_on call.
		 */
		int m_run_id{ 0 };

		/*!
		 * \name Data sources-related part of controller's data.
		 * \{
		 */
		//! Head of data sources list.
		stats::source_t * m_head = { nullptr };
		//! Tail of data sources list.
		stats::source_t * m_tail = { nullptr };

		std::chrono::steady_clock::duration m_distribution_period{
				default_distribution_period() };
		/*!
		 * \}
		 */

		static std::chrono::steady_clock::duration
		very_small_timeout()
			{
				return std::chrono::milliseconds{1};
			}

		//! Actual distribution of the current statistics.
		std::chrono::steady_clock::duration
		distribute_current_data()
			{
				auto started_at = std::chrono::steady_clock::now();

				send< so_5::stats::messages::distribution_started >(
						m_distribution_mbox );

				auto s = m_head;
				while( s )
					{
						s->distribute( m_distribution_mbox );

						s = source_list_next( *s );
					}

				send< so_5::stats::messages::distribution_finished >(
						m_distribution_mbox );

				return std::chrono::steady_clock::now() - started_at;
			}

		//! Helper method for sending next instance of next_turn message.
		void
		send_next_message(
			std::chrono::steady_clock::duration pause,
			const int run_id )
			{
				send_delayed< next_turn >(
						m_env,
						m_next_turn_mbox,
						pause,
						outliving_mutable( *this ),
						run_id );
			}
	};

} /* namespace st_reusable_stuff */

} /* namespace env_infrastructures */

} /* namespace so_5 */


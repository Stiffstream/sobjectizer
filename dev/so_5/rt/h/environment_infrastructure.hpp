/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Interface of environment_infrastructure entity.
 *
 * \since
 * v.5.5.19
 */

#pragma once

#include <so_5/rt/h/agent_coop.hpp>

#include <so_5/rt/stats/h/controller.hpp>
#include <so_5/rt/stats/h/repository.hpp>

#include <so_5/h/timers.hpp>

#include <so_5/h/declspec.hpp>

#include <memory>
#include <functional>

namespace so_5 {

class environment_infrastructure_t;
/*!
 * \brief A type for deleter for environment_infrastructure objects.
 *
 * \since
 * v.5.5.19
 */
using environment_infrastructure_deleter_fnptr_t =
	void (*)(environment_infrastructure_t *);

//
// environment_infrastructure_t
//
/*!
 * \brief An interface for environment_infrastructure entity.
 *
 * Environment infrastructure implements several important parts of
 * SObjectizer's Environment:
 *
 * - repository of registered cooperations (including actions for
 *   final cooperation deregistration);
 * - default dispatcher;
 * - timer thread or manager;
 * - run-time stats_controller and stats_repository.
 *
 * This class defines the interface of environment_infrastructure.
 * All environment_infrastructure implementations must inherit this interface.
 *
 * \since
 * v.5.5.19
 */
class SO_5_TYPE environment_infrastructure_t
	{
	public :
		environment_infrastructure_t() = default;

		environment_infrastructure_t(
			const environment_infrastructure_t & ) = delete;
		environment_infrastructure_t(
			environment_infrastructure_t && ) = delete;

		environment_infrastructure_t &
		operator=( const environment_infrastructure_t & ) = delete;
		environment_infrastructure_t &
		operator=( environment_infrastructure_t && ) = delete;

		virtual ~environment_infrastructure_t() SO_5_NOEXCEPT = default;

		//! Default deleter for environment_infrastructure object.
		/*!
		 * This function is intended for simplification of code
		 * where environment_infrastructure_unique_ptr_t must be created
		 * with default deleter.
		 *
		 * For example:
		 * \code
		   auto my_env = new some_user_specific_env_infrastructure(...);
			return so_5::environment_infrastructure_unique_ptr_t( my_env,
				so_5::environment_infrastructure_t::default_deleter() );
		 * \endcode
		 */
		inline static environment_infrastructure_deleter_fnptr_t
		default_deleter() 
			{
				return [](environment_infrastructure_t * p) { delete p; };
			}

		//! Type of function for performing environment initialization actions.
		using env_init_t = std::function< void() >;

		//
		// coop_repository_stats_t
		//
		/*!
		 * \since
		 * 5.5.19
		 *
		 * \brief Statistical data for run-time monitoring of coop repository
		 * content.
		 */
		struct coop_repository_stats_t
			{
				//! Count of registered cooperations.
				std::size_t m_registered_coop_count;
				//! Count of cooperations in the deregistration state.
				std::size_t m_deregistered_coop_count;

				//! Count of registered agents.
				/*!
				 * This quantity includes quantity of agents in registered
				 * cooperations as well as quantity of agents in cooperations
				 * in the deregistration state.
				 */
				std::size_t m_total_agent_count;

				/*!
				 * \since
				 * v.5.5.12
				 *
				 * \brief Count of coops waiting for the final deregistration.
				 */
				std::size_t m_final_dereg_coop_count;
			};

		//! Do actual launch of SObjectizer's Environment.
		/*!
		 * This method is called as a final step of environment lauch procedure.
		 *
		 * Stats_controller, all additional layers and dispatchers are already
		 * started before calling on this method.
		 *
		 * This method must lock the current thread and return only when
		 * SObjectizer's Environment finishes its work.
		 */
		virtual void
		launch(
			//! Actions from Environment which must be performed to
			//! complete launch procedure.
			env_init_t init_fn ) = 0;

		//! Initiate a signal for shutdown of Environment.
		virtual void
		stop() = 0;

		//! Register new cooperation.
		virtual void
		register_coop(
			//! Cooperation to be registered.
			coop_unique_ptr_t coop ) = 0;

		//! Deregister cooperation.
		virtual void
		deregister_coop(
			//! Cooperation name which being deregistered.
			nonempty_name_t name,
			//! Deregistration reason.
			coop_dereg_reason_t dereg_reason ) = 0;

		//! Notification about cooperation for which the final dereg step
		//! can be performed.
		virtual void
		ready_to_deregister_notify(
			//! A cooperation which can be finaly deregistered.
			coop_t * coop ) = 0;

		//! Do final actions of the cooperation deregistration.
		/*!
		 * \retval true there are some live cooperations.
		 * \retval false there is no more live cooperations.
		 */
		virtual bool
		final_deregister_coop(
			//! Cooperation name to be deregistered.
			/*!
			 * \note
			 * Cooperation name must be passed by value because
			 * reference can become invalid during work of this method.
			 */
			std::string coop_name ) = 0;

		//! Initiate a timer (delayed or periodic message).
		virtual so_5::timer_id_t
		schedule_timer(
			const std::type_index & type_wrapper,
			const message_ref_t & msg,
			const mbox_t & mbox,
			std::chrono::steady_clock::duration pause,
			std::chrono::steady_clock::duration period ) = 0;

		//! Initiate a delayed message.
		virtual void
		single_timer(
			const std::type_index & type_wrapper,
			const message_ref_t & msg,
			const mbox_t & mbox,
			std::chrono::steady_clock::duration pause ) = 0;

		//! Get stats controller for the environment.
		/*!
		 * \note
		 * An instance of stats_controller is created and destroyed by
		 * environment_infrastructure object. But stats_controller is started and
		 * stopped by environment object itself. It means that stats_controller
		 * must be constructed and must be ready to use just after creation of
		 * environment_infrastructure instance and long before invocation of
		 * launch() method.
		 */
		virtual ::so_5::stats::controller_t &
		stats_controller() SO_5_NOEXCEPT = 0;

		//! Get stats repository for the environment.
		/*!
		 * \note
		 * The same rules apply as for stats_controller() method.
		 */
		virtual ::so_5::stats::repository_t &
		stats_repository() SO_5_NOEXCEPT = 0;

		//! Get the default dispatcher.
		virtual dispatcher_t &
		query_default_dispatcher() = 0;

		//! Query run-time statistics for cooperation repository.
		virtual coop_repository_stats_t
		query_coop_repository_stats() = 0;

		//! Query run-time statistics for timer (thread or manager).
		virtual timer_thread_stats_t
		query_timer_thread_stats() = 0;

		//! Create a binder for the default dispatcher.
		virtual disp_binder_unique_ptr_t
		make_default_disp_binder() = 0;
	};

//
// environment_infrastructure_unique_ptr_t
//
/*!
 * \brief A type of unique pointer for environment_infrastructure entity.
 *
 * \note
 * A deleter should be specified for entity object. It makes possible
 * to create unique pointers for objects which must not be delete via
 * delete operator. For example:
 * \code
	// Object is created on stack. Need not to be deleted.
	my_special_infrastructure_t my( ... );
	so_5::launch( [](so_5::environment_t & env) {...},
		[&](so_5::environment_params_t & p) {
			p.environment_infrastructure(
				[&](so_5::environment_t &, so_5::environment_params_t &) {
					return so_5::environment_infrastructure_unique_ptr_t(
							&my, [](so_5::environment_infrastructure_t *){} );
				} );
			...
		} );
 * \endcode
 *
 * \since
 * v.5.5.19
 */
using environment_infrastructure_unique_ptr_t =
		std::unique_ptr<
				environment_infrastructure_t,
				environment_infrastructure_deleter_fnptr_t >;

//
// environment_infrastructure_factory_t
//
/*!
 * \brief Type of factory for environment infrastructure.
 *
 * A factory must be a function with the following format:
 * \code
 * environment_infrastructure_unique_ptr_t
 * factory(
 * 	// Environment for which env_infrastructure will be created.
 * 	environment_t & env,
 * 	// Parameters for that environment.
 * 	environment_params_t & params,
 * 	// Message box for distribution of run-time statistics.
 * 	mbox_t stats_distribution_mbox );
 * \endcode
 *
 * \since
 * v.5.5.19
 */
using environment_infrastructure_factory_t =
		std::function<
				environment_infrastructure_unique_ptr_t(
						environment_t &,
						environment_params_t &,
						mbox_t) >;

} /* namespace so_5 */


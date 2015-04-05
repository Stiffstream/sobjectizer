/*
	SObjectizer 5.
*/

/*!
	\file
	\brief A class for a part of the agent/environment functionality.
*/

#pragma once

#include <map>
#include <memory>
#include <set>
#include <string>
#include <mutex>
#include <condition_variable>

#include <so_5/h/exception.hpp>

#include <so_5/rt/h/agent.hpp>
#include <so_5/rt/h/agent_coop.hpp>
#include <so_5/rt/h/coop_listener.hpp>

#include <so_5/rt/impl/coop_dereg/h/coop_dereg_executor_thread.hpp>

#include <so_5/rt/stats/h/repository.hpp>

namespace so_5
{

namespace rt
{

namespace impl
{

namespace agent_core_details
{

class deregistration_processor_t;

} /* namespace agent_core_details */

//
// agent_coop_private_iface_t
//
/*!
 * \since v.5.2.3
 * \brief A special class for accessing private members of agent_coop.
 */
class agent_coop_private_iface_t
{
	public :
		inline static void
		do_deregistration_specific_actions(
			agent_coop_t & coop,
			coop_dereg_reason_t dereg_reason )
		{
			coop.do_deregistration_specific_actions(
					std::move( dereg_reason ) );
		}

		inline static agent_coop_t *
		parent_coop_ptr( const agent_coop_t & coop )
		{
			return coop.parent_coop_ptr();
		}

		inline static coop_reg_notificators_container_ref_t
		reg_notificators( const agent_coop_t & coop )
		{
			return coop.reg_notificators();
		}

		inline static coop_dereg_notificators_container_ref_t
		dereg_notificators( const agent_coop_t & coop )
		{
			return coop.dereg_notificators();
		}

		inline static coop_dereg_reason_t
		dereg_reason( const agent_coop_t & coop )
		{
			return coop.dereg_reason();
		}
};

//
// agent_core_stats_t
//
/*!
 * \since 5.5.4
 * \brief Statistical data for run-time monitoring.
 */
struct agent_core_stats_t
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
	};

//
// agent_core_t
//

//! A class for a part of the agent/environment functionality.
class agent_core_t
{
		agent_core_t( const agent_core_t & );
		void
		operator = ( const agent_core_t & );

		friend class so_5::rt::impl::agent_core_details::
				deregistration_processor_t;

	public:
		explicit agent_core_t(
			//! SObjectizer Environment.
			environment_t & so_environment_impl,
			//! Cooperation action listener.
			coop_listener_unique_ptr_t coop_listener );

		~agent_core_t();

		//! Do initialization.
		void
		start();

		//! Finish work.
		/*!
		 * Initiates deregistration of all agents. Waits for complete
		 * deregistration for all of them. Waits for termination of
		 * cooperation deregistration thread.
		 */
		void
		finish();

		//! Register cooperation.
		void
		register_coop(
			//! Cooperation to be registered.
			agent_coop_unique_ptr_t agent_coop );

		//! Deregister cooperation.
		void
		deregister_coop(
			//! Cooperation name which being deregistered.
			const nonempty_name_t & name,
			//! Deregistration reason.
			coop_dereg_reason_t dereg_reason );

		//! Notification about readiness of the cooperation deregistration.
		void
		ready_to_deregister_notify(
			agent_coop_t * coop );

		//! Do final actions of the cooperation deregistration.
		/*!
		 * \retval true there are some live cooperations.
		 * \retval false there is no more live cooperations.
		 */
		bool
		final_deregister_coop(
			//! Cooperation name to be deregistered.
			/*!
			 * \note Cooperation name must be passed by value because
			 * reference can become invalid during work of this method.
			*/
			const std::string coop_name );

		//! Initiate start of the cooperation deregistration.
		void
		start_deregistration();

		//! Wait for a signal about start of the cooperation deregistration.
		void
		wait_for_start_deregistration();

		//! Deregisted all cooperations.
		/*!
		 * All cooperations will be deregistered at the SObjectizer shutdown.
		*/
		void
		deregister_all_coop();

		//! Wait for end of all cooperations deregistration.
		void
		wait_all_coop_to_deregister();

		/*!
		 * \since v.5.5.0
		 * \brief Access to SObjectizer Environment.
		 *
		 * \note This access is necessary to use error_logger for
		 * logging error messages.
		 */
		environment_t &
		environment();

		/*!
		 * \since v.5.5.4
		 * \brief Get the current statistic for run-time monitoring.
		 */
		agent_core_stats_t
		query_stats();

	private:
		//! Typedef for map from cooperation name to the cooperation.
		typedef std::map<
				std::string,
				agent_coop_ref_t >
			coop_map_t;

		/*!
		 * \since v.5.2.3
		 * \brief Typedef for pair of names of parent and child cooperations.
		 *
		 * \a first -- name of parent.
		 * \a second -- name of child.
		 */
		typedef std::pair< std::string, std::string >
			parent_child_coop_names_t;

		/*!
		 * \since v.5.2.3
		 * \brief Typedef for set of parent-child names pairs.
		 */
		typedef std::set< parent_child_coop_names_t >
			parent_child_coop_relation_t;

		/*!
		 * \since v.5.2.3
		 * \brief Information for deregistration notification.
		 */
		struct info_for_dereg_notification_t
		{
			coop_dereg_reason_t m_reason;
			coop_dereg_notificators_container_ref_t m_notificators;

			info_for_dereg_notification_t()
			{}

			info_for_dereg_notification_t(
				coop_dereg_reason_t reason,
				coop_dereg_notificators_container_ref_t notificators )
				:	m_reason( std::move( reason ) )
				,	m_notificators( notificators )
			{}

			info_for_dereg_notification_t(
				const info_for_dereg_notification_t & info )
				:	m_reason( info.m_reason )
				,	m_notificators( info.m_notificators )
			{}

			info_for_dereg_notification_t(
				info_for_dereg_notification_t && info )
				:	m_reason( std::move( info.m_reason ) )
				,	m_notificators( std::move( info.m_notificators ) )
			{}

			info_for_dereg_notification_t &
			operator=( info_for_dereg_notification_t o )
			{
				o.swap( *this );
				return *this;
			}

			void
			swap( info_for_dereg_notification_t & o )
			{
				m_reason.swap( o.m_reason );
				m_notificators.swap( o.m_notificators );
			}
		};
		
		/*!
		 * \since v.5.5.4
		 * \brief Result of final remove of a cooperation from
		 * map of deregistered cooperations.
		 *
		 * \note It is necessary to destroy agent_coop object when
		 * agent_core_t is unlocked. It agent_coop is destroyed when
		 * agent_core_t is locked then there is a possibility for a deadlock:
		 * - run-time monitoring thread can wait on agent_core_t mutex
		 *   (but the lock of run-time monitoring thread is acquired);
		 * - private dispatcher for cooperation can be destroyed and its
		 *   data sources will wait on the mutex of run-time monitoring thread.
		 */
		struct final_remove_result_t
			{
				//! Cooperation to be destroyed.
				agent_coop_ref_t m_coop;
				//! Deregistration notifications.
				info_for_dereg_notification_t m_notifications;

				//! Empty constructor.
				final_remove_result_t()
					{}

				//! Initializing constructor.
				final_remove_result_t(
					agent_coop_ref_t coop,
					info_for_dereg_notification_t notifications )
					:	m_coop( std::move( coop ) )
					,	m_notifications( std::move( notifications ) )
					{}

				//! Copy constructor.
				final_remove_result_t(
					const final_remove_result_t & o )
					:	m_coop( o.m_coop )
					,	m_notifications( o.m_notifications )
					{}

				//! Move constructor.
				final_remove_result_t(
					final_remove_result_t && o )
					:	m_coop( std::move( o.m_coop ) )
					,	m_notifications( std::move( o.m_notifications ) )
					{}

				//! Copy operator.
				final_remove_result_t &
				operator=( final_remove_result_t o )
					{
						o.swap( *this );
						return *this;
					}

				//! Swap operation.
				void
				swap( final_remove_result_t & o )
					{
						m_coop.swap( o.m_coop );
						m_notifications.swap( o.m_notifications );
					}
			};

		//! SObjectizer Environment to work with.
		environment_t & m_so_environment;

		//! Lock for operations on cooperations.
		std::mutex m_coop_operations_lock;

		//! Condition variable for the deregistration start indication.
		std::condition_variable m_deregistration_started_cond;

		//! Condition variable for the deregistration finish indication.
		std::condition_variable m_deregistration_finished_cond;

		//! Indicator for all cooperation deregistration.
		bool m_deregistration_started;

		//! Map of registered cooperations.
		coop_map_t m_registered_coop;

		//! Map of cooperations being deregistered.
		coop_map_t m_deregistered_coop;

		//! Total count of agents.
		/*!
		 * \since v.5.5.4
		 */
		std::size_t m_total_agent_count;

		//! Cooperation deregistration thread.
		coop_dereg::coop_dereg_executor_thread_t m_coop_dereg_executor;

		//! Cooperation actions listener.
		coop_listener_unique_ptr_t m_coop_listener;

		/*!
		 * \since v.5.2.3
		 * \brief Information about parent and child cooperations
		 * relationship.
		 */
		parent_child_coop_relation_t m_parent_child_relations;

		/*!
		 * \since v.5.2.3
		 * \brief Ensures that name of new cooperation is unique.
		 */
		void
		ensure_new_coop_name_unique(
			const std::string & coop_name ) const;

		/*!
		 * \since v.5.2.3
		 * \brief Checks that parent cooperation is registered if its name
		 * is set for the cooperation specified.
		 *
		 * \retval nullptr if no parent cooperation name set. Otherwise the
		 * pointer to parent cooperation is returned.
		 */
		agent_coop_t *
		find_parent_coop_if_necessary(
			const agent_coop_t & coop_to_be_registered ) const;

		/*!
		 * \since v.5.2.3
		 * \brief Next step of cooperation registration.
		 *
		 * Initiate cooperation registration actions and
		 * store cooperation info in registered cooperations map.
		 */
		void
		next_coop_reg_step__update_registered_coop_map(
			//! Cooperation to be registered.
			const agent_coop_ref_t & coop_ref,
			//! Pointer to parent cooperation.
			//! Equal to nullptr if \a coop has no parent.
			agent_coop_t * parent_coop_ptr );

		/*!
		 * \since v.5.2.3
		 * \brief Next step of cooperation registration.
		 *
		 * Updates information about parent-child cooperation relationship
		 * and goes further.
		 */
		void
		next_coop_reg_step__parent_child_relation(
			//! Cooperation to be registered.
			const agent_coop_ref_t & coop,
			//! Pointer to parent cooperation.
			//! Equal to nullptr if \a coop has no parent.
			agent_coop_t * parent_coop_ptr );

		/*!
		 * \since v.5.2.3
		 * \brief Do final action for cooperation deregistration.
		 *
		 * If parent cooperation exists then parent-child relation
		 * is handled appropriatelly.
		 *
		 * Information about cooperation is removed from m_deregistered_coop.
		 */
		final_remove_result_t
		finaly_remove_cooperation_info(
			const std::string & coop_name );

		/*!
		 * \since v.5.2.3
		 * \brief Do all job related to sending notification about
		 * cooperation registration.
		 */
		void
		do_coop_reg_notification_if_necessary(
			const std::string & coop_name,
			const coop_reg_notificators_container_ref_t & notificators ) const;

		/*!
		 * \since v.5.2.3
		 * \brief Do all job related to sending notification about
		 * cooperation deregistration.
		 */
		void
		do_coop_dereg_notification_if_necessary(
			const std::string & coop_name,
			const info_for_dereg_notification_t & notification_info ) const;
};

} /* namespace impl */

} /* namespace rt */

} /* namespace so_5 */

/*
	SObjectizer 5.
*/

/*!
 * \file
 * \brief Basic part of coop_repository functionality.
 *
 * \since
 * v.5.5.19
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
#include <so_5/rt/h/environment_infrastructure.hpp>

#include <so_5/rt/stats/h/repository.hpp>

#if defined( SO_5_MSVC )
	#pragma warning(push)
	#pragma warning(disable: 4251)
#endif

namespace so_5
{

namespace impl
{

namespace coop_repository_details
{

class deregistration_processor_t;

} /* namespace coop_repository_details */

//
// coop_private_iface_t
//
/*!
 * \since
 * v.5.2.3
 *
 * \brief A special class for accessing private members of agent_coop.
 */
class coop_private_iface_t
{
	public :
		inline static void
		do_deregistration_specific_actions(
			coop_t & coop,
			coop_dereg_reason_t dereg_reason )
		{
			coop.do_deregistration_specific_actions(
					std::move( dereg_reason ) );
		}

		inline static void
		do_registration_specific_actions(
			coop_t & coop,
			coop_t * parent_coop )
		{
			coop.do_registration_specific_actions( parent_coop );
		}

		inline static coop_t *
		parent_coop_ptr( const coop_t & coop )
		{
			return coop.parent_coop_ptr();
		}

		inline static coop_reg_notificators_container_ref_t
		reg_notificators( const coop_t & coop )
		{
			return coop.reg_notificators();
		}

		inline static coop_dereg_notificators_container_ref_t
		dereg_notificators( const coop_t & coop )
		{
			return coop.dereg_notificators();
		}

		inline static coop_dereg_reason_t
		dereg_reason( const coop_t & coop )
		{
			return coop.dereg_reason();
		}
};

//
// coop_repository_basis_t
//

/*!
 * \brief A basic part for various implementations of coop_repository.
 *
 * \since
 * v.5.5.19
 */
class SO_5_TYPE coop_repository_basis_t
{
	coop_repository_basis_t( const coop_repository_basis_t & ) = delete;

	friend class so_5::impl::coop_repository_details::
			deregistration_processor_t;

public:
	coop_repository_basis_t(
		//! SObjectizer Environment.
		environment_t & so_environment_impl,
		//! Cooperation action listener.
		coop_listener_unique_ptr_t coop_listener );

	//! Register cooperation.
	void
	register_coop(
		//! Cooperation to be registered.
		coop_unique_ptr_t agent_coop );

	//! Deregister cooperation.
	void
	deregister_coop(
		//! Cooperation name which being deregistered.
		nonempty_name_t name,
		//! Deregistration reason.
		coop_dereg_reason_t dereg_reason );

	/*!
	 * Type for return value of final_deregister_coop method.
	 *
	 * This method must return two boolean values:
	 * - the first value equals to true if there is at least one live coop;
	 * - the second value equals to true if total deregistration is in
	 *   progress and there is no more live coops;
	 *
	 * \since
	 * v.5.5.19
	 */
	struct final_deregistration_resul_t
		{
			const bool m_has_live_coop;
			const bool m_total_deregistration_completed;

			final_deregistration_resul_t(
				bool has_live_coop,
				bool total_deregistration_completed )
				:	m_has_live_coop( has_live_coop )
				,	m_total_deregistration_completed( total_deregistration_completed )
				{}
		};

	//! Do final actions of the cooperation deregistration.
	/*!
	 * \retval true there are some live cooperations.
	 * \retval false there is no more live cooperations.
	 */
	final_deregistration_resul_t
	final_deregister_coop(
		//! Cooperation name to be deregistered.
		/*!
		 * \attention
		 * This must be a copy of cooperation name.
		 * Because if it is a reference to the cooperation name that
		 * reference will become invalid during execution of this method.
		 */
		std::string coop_name );

	//! Deregisted all cooperations.
	/*!
	 * All cooperations will be deregistered at the SObjectizer shutdown.
	 *
	 * \return Count of coops to be completely deregistered.
	 */
	std::size_t
	deregister_all_coop() SO_5_NOEXCEPT;

	/*!
	 * \brief Result of initiation of total deregistration process.
	 *
	 * \since
	 * v.5.5.19
	 */
	enum class initiate_deregistration_result_t
		{
			initiated_first_time,
			already_in_progress
		};

	/*!
	 * \brief Try to start total deregistration process.
	 *
	 * \note
	 * This method doesn't call deregister_all_coop().
	 * If it is necessary to start deregistration process with
	 * deregistration of all already registered coops then
	 * deregister_all_coop() must be used.
	 *
	 * \since
	 * v.5.5.19
	 */
	initiate_deregistration_result_t
	initiate_deregistration();

	/*!
	 * \since
	 * v.5.5.0
	 *
	 * \brief Access to SObjectizer Environment.
	 *
	 * \note This access is necessary to use error_logger for
	 * logging error messages.
	 */
	environment_t &
	environment();

	/*!
	 * \since
	 * v.5.5.4
	 *
	 * \brief Get the current statistic for run-time monitoring.
	 */
	environment_infrastructure_t::coop_repository_stats_t
	query_stats();

	/*!
	 * Get access to repository's mutex.
	 *
	 * \since
	 * v.5.5.19
	 */
	std::mutex &
	lock()
		{
			return m_coop_operations_lock;
		}

protected:
	//! Typedef for map from cooperation name to the cooperation.
	typedef std::map< std::string, coop_ref_t > coop_map_t;

	/*!
	 * \since
	 * v.5.2.3
	 *
	 * \brief Typedef for pair of names of parent and child cooperations.
	 *
	 * \a first -- name of parent.
	 * \a second -- name of child.
	 */
	typedef std::pair< std::string, std::string >
		parent_child_coop_names_t;

	/*!
	 * \since
	 * v.5.2.3
	 *
	 * \brief Typedef for set of parent-child names pairs.
	 */
	typedef std::set< parent_child_coop_names_t >
		parent_child_coop_relation_t;

	/*!
	 * \since
	 * v.5.2.3
	 *
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
	 * \since
	 * v.5.5.4
	 *
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
			coop_ref_t m_coop;
			//! Deregistration notifications.
			info_for_dereg_notification_t m_notifications;

			//! Empty constructor.
			final_remove_result_t()
				{}

			//! Initializing constructor.
			final_remove_result_t(
				coop_ref_t coop,
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

	//! Indicator for all cooperation deregistration.
	bool m_deregistration_started;

	//! Map of registered cooperations.
	coop_map_t m_registered_coop;

	//! Map of cooperations being deregistered.
	coop_map_t m_deregistered_coop;

	//! Total count of agents.
	/*!
	 * \since
	 * v.5.5.4
	 */
	std::size_t m_total_agent_count;

	//! Cooperation actions listener.
	coop_listener_unique_ptr_t m_coop_listener;

	/*!
	 * \since
	 * v.5.2.3
	 *
	 * \brief Information about parent and child cooperations
	 * relationship.
	 */
	parent_child_coop_relation_t m_parent_child_relations;

	/*!
	 * \since
	 * v.5.2.3
	 *
	 * \brief Ensures that name of new cooperation is unique.
	 */
	void
	ensure_new_coop_name_unique(
		const std::string & coop_name ) const;

	/*!
	 * \since
	 * v.5.2.3
	 *
	 * \brief Checks that parent cooperation is registered if its name
	 * is set for the cooperation specified.
	 *
	 * \retval nullptr if no parent cooperation name set. Otherwise the
	 * pointer to parent cooperation is returned.
	 */
	coop_t *
	find_parent_coop_if_necessary(
		const coop_t & coop_to_be_registered ) const;

	/*!
	 * \since
	 * v.5.2.3
	 *
	 * \brief Next step of cooperation registration.
	 *
	 * Initiate cooperation registration actions and
	 * store cooperation info in registered cooperations map.
	 */
	void
	next_coop_reg_step__update_registered_coop_map(
		//! Cooperation to be registered.
		const coop_ref_t & coop_ref,
		//! Pointer to parent cooperation.
		//! Equal to nullptr if \a coop has no parent.
		coop_t * parent_coop_ptr );

	/*!
	 * \since
	 * v.5.2.3
	 *
	 * \brief Next step of cooperation registration.
	 *
	 * Updates information about parent-child cooperation relationship
	 * and goes further.
	 */
	void
	next_coop_reg_step__parent_child_relation(
		//! Cooperation to be registered.
		const coop_ref_t & coop,
		//! Pointer to parent cooperation.
		//! Equal to nullptr if \a coop has no parent.
		coop_t * parent_coop_ptr );

	/*!
	 * \since
	 * v.5.2.3
	 *
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
	 * \since
	 * v.5.2.3
	 *
	 * \brief Do all job related to sending notification about
	 * cooperation registration.
	 */
	void
	do_coop_reg_notification_if_necessary(
		const std::string & coop_name,
		const coop_reg_notificators_container_ref_t & notificators ) const;

	/*!
	 * \since
	 * v.5.2.3
	 *
	 * \brief Do all job related to sending notification about
	 * cooperation deregistration.
	 */
	void
	do_coop_dereg_notification_if_necessary(
		const std::string & coop_name,
		const info_for_dereg_notification_t & notification_info ) const;
};

} /* namespace impl */

} /* namespace so_5 */

#if defined( SO_5_MSVC )
	#pragma warning(pop)
#endif

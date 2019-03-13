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

#include <so_5/impl/coop_private_iface.hpp>

#include <so_5/exception.hpp>

#include <so_5/agent.hpp>
#include <so_5/coop_listener.hpp>
#include <so_5/environment_infrastructure.hpp>

#include <so_5/stats/repository.hpp>

#include <map>
#include <memory>
#include <set>
#include <string>
#include <mutex>
#include <condition_variable>

#if defined( SO_5_MSVC )
	#pragma warning(push)
	#pragma warning(disable: 4251)
#endif

namespace so_5
{

namespace impl
{

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
	coop_repository_basis_t( coop_repository_basis_t && ) = delete;

public:
	coop_repository_basis_t(
		//! SObjectizer Environment.
		outliving_reference_t< environment_t > environment,
		//! Cooperation action listener.
		coop_listener_unique_ptr_t coop_listener );

//FIXME: document this!
	SO_5_NODISCARD
	coop_unique_ptr_t
	make_coop(
		coop_handle_t parent,
		disp_binder_shptr_t default_binder );

	//! Register cooperation.
	coop_handle_t
	register_coop(
		//! Cooperation to be registered.
		coop_unique_ptr_t agent_coop );

	//! Deregister cooperation.
	void
	deregister_coop(
		//! Cooperation which being deregistered.
		coop_handle_t coop,
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
		coop_shptr_t coop );

	//! Deregisted all cooperations.
	/*!
	 * All cooperations will be deregistered at the SObjectizer shutdown.
	 *
	 * \return Count of coops to be completely deregistered.
	 */
	std::size_t
	deregister_all_coop() noexcept;

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

protected:
	outliving_reference_t< environment_t > m_env;
};

} /* namespace impl */

} /* namespace so_5 */

#if defined( SO_5_MSVC )
	#pragma warning(pop)
#endif

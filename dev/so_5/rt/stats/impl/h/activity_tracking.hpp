/*
 * SObjectizer-5
 */

/*!
 * \file
 * \since
 * v.5.5.18
 *
 * \brief Details for implementation of activity tracking.
 */

#pragma once

#include <so_5/h/types.hpp>
#include <so_5/h/spinlocks.hpp>

#include <so_5/rt/stats/h/work_thread_activity.hpp>

namespace so_5
{

namespace stats
{

namespace activity_tracking_stuff {

/*!
 * \since
 * v.5.5.18
 *
 * \brief Various traits of activity tracking implementation.
 */
struct traits
	{
		//! Type of lock object to be used for synchronization of
		//! activity tracking data.
		using lock_t = default_spinlock_t;
	};

/*!
 * \brief An analog of std::lock_guard but without actual locking actions.
 *
 * This class is inteded to be used with different lock policies for
 * stats_collector_t.
 *
 * \since
 * v.5.5.18
 */
template< typename L >
struct no_actual_lock
	{
		no_actual_lock( L & ) { /* Do nothing. */ }
	};

/*!
 * \brief Default locking policy for stats_collector_t.
 *
 * Performs actual locking on start/stop and take_activity operations.
 *
 * \since
 * v.5.5.18
 */
template< typename LOCK_HOLDER >
struct default_lock_policy
	{
		using start_stop_lock_t = std::lock_guard< LOCK_HOLDER >;
		using take_stats_lock_t = std::lock_guard< LOCK_HOLDER >;
	};

/*!
 * \brief A custom locking policy for stats_collector_t.
 *
 * Performs actual locking only on take_activity operation.
 *
 * \since
 * v.5.5.18
 */
template< typename LOCK_HOLDER >
struct no_lock_at_start_stop_policy
	{
		using start_stop_lock_t = no_actual_lock< LOCK_HOLDER >;
		using take_stats_lock_t = std::lock_guard< LOCK_HOLDER >;
	};

/*!
 * \brief Base for the case of internal stats lock.
 *
 * \since
 * v.5.5.18
 */
class internal_lock
	{
		traits::lock_t m_lock;
	public :
		using start_stop_lock_t = std::lock_guard< internal_lock >;
		using take_stats_lock_t = std::lock_guard< internal_lock >;

		internal_lock() {}

		void lock() { m_lock.lock(); }
		void unlock() { m_lock.unlock(); }
	};

/*!
 * \brief Base for the case of externals stats lock.
 *
 * \since
 * v.5.5.18
 */
template<
	typename LOCK_TYPE = traits::lock_t,
	template<class> class LOCK_POLICY = default_lock_policy >
class external_lock
	{
		LOCK_TYPE & m_lock;
	public :
		using start_stop_lock_t =
				typename LOCK_POLICY< external_lock >::start_stop_lock_t;
		using take_stats_lock_t =
				typename LOCK_POLICY< external_lock >::take_stats_lock_t;

		external_lock( LOCK_TYPE & lock ) : m_lock( lock ) {}

		void lock() { m_lock.lock(); }
		void unlock() { m_lock.unlock(); }
	};


/*!
 * \brief Helper for collecting activity stats.
 *
 * \since
 * v.5.5.18
 */
template< typename LOCK_HOLDER >
class stats_collector_t
	{
		using start_stop_lock_t = typename LOCK_HOLDER::start_stop_lock_t;
		using take_stats_lock_t = typename LOCK_HOLDER::take_stats_lock_t;

	public :
		template< typename... ARGS >
		stats_collector_t( ARGS && ...args )
			:	m_lock_holder( std::forward<ARGS>(args)... )
			{}

		void
		start()
			{
				start_stop_lock_t lock{ m_lock_holder };

				m_is_in_working = true;
				m_work_started_at = so_5::stats::clock_type_t::now();
				m_work_activity.m_count += 1;
			}

		void
		stop()
			{
				start_stop_lock_t lock{ m_lock_holder };

				m_is_in_working = false;
				so_5::stats::details::update_stats_from_current_time(
						m_work_activity,
						m_work_started_at );
			}

		so_5::stats::activity_stats_t
		take_stats()
			{
				so_5::stats::activity_stats_t result;
				bool is_in_working{ false };
				so_5::stats::clock_type_t::time_point work_started_at;

				{
					take_stats_lock_t lock{ m_lock_holder };

					result = m_work_activity;
					if( true == (is_in_working = m_is_in_working) )
						work_started_at = m_work_started_at;
				}

				if( is_in_working )
					so_5::stats::details::update_stats_from_current_time(
							result,
							work_started_at );

				return result;
			}

	private :
		LOCK_HOLDER m_lock_holder;

		//! A flag for indicating work activity.
		bool m_is_in_working{ false };

		//! A time point when current activity started.
		so_5::stats::clock_type_t::time_point m_work_started_at;

		//! A statistics for work activity.
		so_5::stats::activity_stats_t m_work_activity{};
	};

/*!
 * \brief Helper function for creation of dispatcher with respect
 * to activity tracking flag in dispatcher params and in Environment's
 * params.
 *
 * \since
 * v.5.5.18
 */
template<
	typename COMMON_DISP_IFACE_TYPE,
	typename DISP_NO_TRACKING,
	typename DISP_WITH_TRACKING,
	typename ENV,
	typename DISP_PARAMS,
	typename... ARGS >
std::unique_ptr< COMMON_DISP_IFACE_TYPE >
create_appropriate_disp(
	ENV & env,
	const DISP_PARAMS & disp_params,
	ARGS && ...args )
	{
		std::unique_ptr< COMMON_DISP_IFACE_TYPE > disp;

		auto tracking = disp_params.work_thread_activity_tracking();
		if( work_thread_activity_tracking_t::unspecified == tracking )
			tracking = env.work_thread_activity_tracking();

		if( work_thread_activity_tracking_t::on == tracking )
			disp.reset(
				new DISP_WITH_TRACKING{ std::forward<ARGS>(args)... } );
		else
			disp.reset(
				new DISP_NO_TRACKING{ std::forward<ARGS>(args)... } );

		return disp;
	}

} /* namespace activity_tracking_stuff */

} /* namespace stats */

} /* namespace so_5 */


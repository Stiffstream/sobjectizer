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
template< typename Lock_Holder >
struct default_lock_policy
	{
		using start_stop_lock_t = std::lock_guard< Lock_Holder >;
		using take_stats_lock_t = std::lock_guard< Lock_Holder >;
	};

/*!
 * \brief A custom locking policy for stats_collector_t.
 *
 * Performs actual locking only on take_activity operation.
 *
 * \since
 * v.5.5.18
 */
template< typename Lock_Holder >
struct no_lock_at_start_stop_policy
	{
		using start_stop_lock_t = no_actual_lock< Lock_Holder >;
		using take_stats_lock_t = std::lock_guard< Lock_Holder >;
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
	typename Lock_Type = traits::lock_t,
	template<class> class Lock_Policy = default_lock_policy >
class external_lock
	{
		Lock_Type & m_lock;
	public :
		using start_stop_lock_t =
				typename Lock_Policy< external_lock >::start_stop_lock_t;
		using take_stats_lock_t =
				typename Lock_Policy< external_lock >::take_stats_lock_t;

		external_lock( Lock_Type & lock ) : m_lock( lock ) {}

		void lock() { m_lock.lock(); }
		void unlock() { m_lock.unlock(); }
	};

/*!
 * \brief A special class for cases where lock is not needed at all.
 *
 * Usage example:
 * \code
class real_activity_tracker_t final
	{
		so_5::stats::activity_tracking_stuff::stats_collector_t<
						so_5::stats::activity_tracking_stuff::null_lock >
				m_waiting{};

		so_5::stats::activity_tracking_stuff::stats_collector_t<
						so_5::stats::activity_tracking_stuff::null_lock >
				m_working{};
		...
	};
 * \endcode
 *
 * \since
 * v.5.5.19
 */
struct null_lock
	{
	public :
		using start_stop_lock_t = no_actual_lock< null_lock >;
		using take_stats_lock_t = no_actual_lock< null_lock >;

		null_lock() {}
	};

/*!
 * \brief Helper for collecting activity stats.
 *
 * \since
 * v.5.5.18
 */
template< typename Lock_Holder >
class stats_collector_t : protected Lock_Holder
	{
		Lock_Holder &
		lock_holder() { return *this; }

	public :
		template< typename... Args >
		stats_collector_t( Args && ...args )
			:	Lock_Holder( std::forward<Args>(args)... )
			{}

		void
		start()
			{
				typename Lock_Holder::start_stop_lock_t lock{ lock_holder() };

				do_start();
			}

		/*!
		 * \brief A helper method for safe start if start method hasn't been
		 * called yet.
		 *
		 * \since
		 * v.5.5.19
		 */
		void
		start_if_not_started()
			{
				typename Lock_Holder::start_stop_lock_t lock{ lock_holder() };

				if( !m_is_in_working )
					do_start();
			}

		void
		stop()
			{
				typename Lock_Holder::start_stop_lock_t lock{ lock_holder() };

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
					typename Lock_Holder::take_stats_lock_t lock{ lock_holder() };

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
		//! A flag for indicating work activity.
		bool m_is_in_working{ false };

		//! A time point when current activity started.
		so_5::stats::clock_type_t::time_point m_work_started_at;

		//! A statistics for work activity.
		so_5::stats::activity_stats_t m_work_activity{};

		void
		do_start()
			{
				m_is_in_working = true;
				m_work_started_at = so_5::stats::clock_type_t::now();
				m_work_activity.m_count += 1;
			}
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
	typename Common_Disp_Iface_Type,
	typename Disp_No_Tracking,
	typename Disp_With_Tracking,
	typename Env,
	typename Disp_Params,
	typename... Args >
std::unique_ptr< Common_Disp_Iface_Type >
create_appropriate_disp(
	Env & env,
	const Disp_Params & disp_params,
	Args && ...args )
	{
		std::unique_ptr< Common_Disp_Iface_Type > disp;

		auto tracking = disp_params.work_thread_activity_tracking();
		if( work_thread_activity_tracking_t::unspecified == tracking )
			tracking = env.work_thread_activity_tracking();

		if( work_thread_activity_tracking_t::on == tracking )
			disp.reset(
				new Disp_With_Tracking{ std::forward<Args>(args)... } );
		else
			disp.reset(
				new Disp_No_Tracking{ std::forward<Args>(args)... } );

		return disp;
	}

} /* namespace activity_tracking_stuff */

} /* namespace stats */

} /* namespace so_5 */


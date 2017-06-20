/*
 * SObjectizer-5
 */

/*!
 * \file
 * \brief Repository of stop_guards.
 *
 * \since
 * v.5.5.19.2
 */

#pragma once

#include <so_5/h/stop_guard.hpp>

#include <so_5/details/h/sync_helpers.hpp>
#include <so_5/details/h/invoke_noexcept_code.hpp>

#include <mutex>
#include <algorithm>
#include <iterator>

namespace so_5 {

namespace impl {

//
// stop_guard_repository_t
//
/*!
 * \brief Repository of stop_guards.
 *
 * \note
 * Performs multitheading defense by very simple way: via std::mutex object.
 *
 * \since
 * v.5.5.19.2
 */
class stop_guard_repository_t
	: public ::so_5::details::lock_holder_detector< std::mutex >::type
	{
	public :
		//! Action which must be performed as result of operation on repository.
		enum class action_t
			{
				//! Nothing to do. Stop operation is not started.
				do_nothing,
				//! Stop operation is started but can't be finished right now.
				wait_for_completion,
				//! Stop operation must be finished.
				do_actual_stop
			};

		stop_guard_repository_t()
			{}
		stop_guard_repository_t( const stop_guard_repository_t & ) = delete;
		stop_guard_repository_t( stop_guard_repository_t && ) = delete;

		//! Setup new stop_guard.
		/*!
		 * \note
		 * Uniqueness of stop_guard is not checked. It means that the same
		 * stop_guard can be added to repository several times.
		 *
		 * \retval stop_guard_t::setup_result_t::ok if stop is not in progress
		 * and new stop_guard has been set up successfully.
		 * \retval stop_guard_t::setup_result_t::stop_already_in_progress new
		 * stop_guard is not set because stop is already in progress.
		 */
		stop_guard_t::setup_result_t
		setup_guard( stop_guard_shptr_t guard )
			{
				return this->lock_and_perform( [&] {
					auto ret_value = stop_guard_t::setup_result_t::ok;
					if( status_t::not_started != m_status )
						ret_value = stop_guard_t::setup_result_t::stop_already_in_progress;
					else
						{
							auto it = std::lower_bound(
									std::begin(m_guards), std::end(m_guards), guard );
							m_guards.insert( it, std::move(guard) );
						}

					return ret_value;
				} );
			}

		//! Remove stop_guard.
		/*!
		 * \retval action_t::do_nothing there is no stop in progress and nothing
		 * need to be done.
		 * \retval action_t::wait_for_completion there is stop in progress but
		 * there are also other stop_guards which do not complete their
		 * actions yet.
		 * \retval action_t::do_actual_stop stop operation needs to be completed.
		 */
		action_t
		remove_guard( stop_guard_shptr_t guard ) SO_5_NOEXCEPT
			{
				return so_5::details::invoke_noexcept_code( [&] {
					return this->lock_and_perform( [&] {
						auto ret_value = action_t::do_nothing;

						const auto it = std::lower_bound(
								std::begin(m_guards),
								std::end(m_guards),
								guard );
						if( it != std::end(m_guards) && *it == guard )
							m_guards.erase(it);

						if( status_t::started == m_status )
							ret_value = m_guards.empty() ?
									action_t::do_actual_stop :
									action_t::wait_for_completion;

						return ret_value;
					} );
				} );
			}

		//! Initiate stop operation.
		/*!
		 * \note
		 * It is safe to call this method when stop is already in progress.
		 *
		 * \retval action_t::wait_for_completion there are some stop_guards
		 * which do not complete their actions yet.
		 * \retval action_t::do_actual_stop stop operation needs to be completed.
		 */
		action_t
		initiate_stop() SO_5_NOEXCEPT
			{
				return so_5::details::invoke_noexcept_code( [&] {
					auto ret_value = action_t::wait_for_completion;

					// Copy of actual guards at this moment.
					guards_container_t guards;

					// The first stage: change status and take a copy
					// of actual guards list if necessary.
					this->lock_and_perform( [&] {
						if( status_t::not_started == m_status )
							{
								// The stop operation is not started yet.
								m_status = status_t::start_in_progress;
								guards = m_guards;
							}
					} );

					// The second stage: calling stop() for all stop_guards.
					// If guards is empty then nothing will be called.
					for( auto & g : guards )
						g->stop();

					// The third stage: check for possibitility to complete
					// the stop operation right now.
					// It is possible that all stop_guards will be removed
					// on the second stage.
					this->lock_and_perform( [&] {
						if( m_guards.empty() )
							{
								m_status = status_t::completed;
								ret_value = action_t::do_actual_stop;
							}
						else
							// The stop operation is started. We must wait
							// while all live stop_guards finish their work.
							m_status = status_t::started;
					} );

					return ret_value;
				} );
			}

	private :
		//! Status of the stop operation.
		enum class status_t
			{
				not_started,
				start_in_progress,
				started,
				completed
			};

		//! The current status of the stop operation
		status_t m_status = status_t::not_started;

		//! Type of the stop_guards list.
		using guards_container_t = std::vector< stop_guard_shptr_t >;

		//! List of actual stop_guards.
		guards_container_t m_guards;
	};

} /* namespace impl */

} /* namespace so_5 */


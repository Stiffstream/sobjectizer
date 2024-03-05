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

#include <so_5/stop_guard.hpp>

#include <so_5/details/sync_helpers.hpp>
#include <so_5/details/invoke_noexcept_code.hpp>

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
			{
				constexpr std::size_t initial_capacity = 4;
				m_guards.reserve( initial_capacity );
				m_container_for_shutdown.reserve( initial_capacity );
			}
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
							// Ensure that m_container_for_shutdown has enough space for
							// updated content of m_guards.
							if( m_guards.size() + 1 > m_container_for_shutdown.capacity() )
								// Just double the capacity.
								// NOTE: don't care about exceptions. If this operation
								// it's not a problem because nothing has been changed yet.
								m_container_for_shutdown.reserve(
										m_container_for_shutdown.capacity() * 2u );

							// Now we can modify the main container.
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
		remove_guard( stop_guard_shptr_t guard ) noexcept
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
		initiate_stop() noexcept
			{
				return so_5::details::invoke_noexcept_code( [&] {
					auto ret_value = action_t::wait_for_completion;

					// The first stage: change status and take a copy
					// of actual guards list if necessary.
					bool need_call_stop = false;
					this->lock_and_perform( [&] {
						if( status_t::not_started == m_status )
							{
								// We expect that there won't be exceptions because:
								//
								// 1. m_container_for_shutdown has enough capacity to
								//    hold all values from m_guards without additional
								//    allocations.
								// 2. std::shared_ptr::operator= is noexcept.
								m_container_for_shutdown = m_guards;
								// The stop operation is not started yet.
								m_status = status_t::start_in_progress;

								need_call_stop = true;
							}
					} );

					// The second stage: calling stop() for all stop_guards.
					if( need_call_stop )
					{
						for( auto & g : m_container_for_shutdown )
						{
							g->stop();
						}

						// We don't need the content of m_container_for_shutdown anymore.
						// It seems that modifying m_container_for_shutdown here is safe,
						// because only one thread can be in this place.
						m_container_for_shutdown.clear();
					}

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

		//! Additional container to be used on shutdown operation.
		/*!
		 * We need a copy of m_guards in initiate_stop(). But the initiate_stop()
		 * method is noexcept and we can't just create a copy of m_guards this way:
		 *
		 * \code
		 * guards_container_t guards_to_stop = m_guards;
		 * \endcode
		 *
		 * because it will require allocations and allocations may throw.
		 *
		 * To make initiate_stop() noexcept we have to reserve space for that
		 * a copy earlier. The m_container_for_shutdown is used for that purpose.
		 *
		 * Every time a new item added to m_guards the capacity of m_container_for_shutdown
		 * is checked. If there is no enough space to copy m_guards to
		 * m_container_for_shutdown then the capacity of m_container_for_shutdown
		 * is increased. Thus we always have space in m_container_for_shutdown
		 * to make a full copy of m_guards.
		 *
		 * \note
		 * In the current implementation of SObjectizer the capacity of
		 * m_container_for_shutdown is always growth but never descreases.
		 *
		 * \since v.5.8.2
		 */
		guards_container_t m_container_for_shutdown;
	};

} /* namespace impl */

} /* namespace so_5 */


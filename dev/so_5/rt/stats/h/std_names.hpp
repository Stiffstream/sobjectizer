/*
 * SObjectizer-5
 */

/*!
 * \since v.5.5.4
 * \file
 * \brief Standard data sources prefixes and suffixes used by SObjectizer.
 */

#pragma once

#include <so_5/rt/stats/h/prefix.hpp>

#include <so_5/h/declspec.hpp>

namespace so_5 {

namespace rt {

namespace stats {

/*!
 * \name Prefixes
 * \{
 */

/*!
 * \since v.5.5.4
 * \brief Prefix of data sources with statistics for cooperations and
 * agents repository.
 */
SO_5_FUNC prefix_t
prefix_coop_repository();

/*!
 * \since v.5.5.4
 * \brief Prefix of data sources with statistics for mboxes repository.
 */
SO_5_FUNC prefix_t
prefix_mbox_repository();

/*!
 * \since v.5.5.4
 * \brief Prefix of data sources with statistics for timer thread.
 */
SO_5_FUNC prefix_t
prefix_timer_thread();
/*!
 * \}
 */

/*!
 * \name Suffixes
 * \{
 */

/*!
 * \since v.5.5.4
 * \brief Suffix for data source with count of registered cooperations.
 */
SO_5_FUNC suffix_t
suffix_coop_reg_count();

/*!
 * \since v.5.5.4
 * \brief Suffix for data source with count of cooperations in the
 * deregistration state.
 */
SO_5_FUNC suffix_t
suffix_coop_dereg_count();

/*!
 * \since v.5.5.4
 * \brief Suffix for data source with count of named mboxes.
 */
SO_5_FUNC suffix_t
suffix_named_mbox_count();

/*!
 * \since v.5.5.4
 * \brief Suffix for data source with count of agents bound to some entity.
 */
SO_5_FUNC suffix_t
suffix_agent_count();

/*!
 * \since v.5.5.4
 * \brief Suffix for data source with count of active groups in
 * an active_group dispatcher.
 */
SO_5_FUNC suffix_t
suffix_disp_active_group_count();

/*!
 * \since v.5.5.4
 * \brief Suffix for data source with count of demands in a working thread event queue.
 */
SO_5_FUNC suffix_t
suffix_work_thread_queue_size();

/*!
 * \since v.5.5.4
 * \brief Suffix for data source with count of work threads for dispatcher.
 */
SO_5_FUNC suffix_t
suffix_disp_thread_count();

/*!
 * \since v.5.5.4
 * \brief Suffix for data source with count of single-shot timers.
 */
SO_5_FUNC suffix_t
suffix_timer_single_shot_count();

/*!
 * \since v.5.5.4
 * \brief Suffix for data source with count of periodic timers.
 */
SO_5_FUNC suffix_t
suffix_timer_periodic_count();

/*!
 * \}
 */

} /* namespace stats */

} /* namespace rt */

} /* namespace so_5 */


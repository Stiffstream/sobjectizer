/*
 * SObjectizer-5
 */

/*!
 * \since
 * v.5.5.4
 *
 * \file
 * \brief Standard data sources prefixes and suffixes used by SObjectizer.
 */

#pragma once

#include <so_5/rt/stats/h/prefix.hpp>

#include <so_5/h/declspec.hpp>

namespace so_5 {

namespace stats {

namespace prefixes {

/*!
 * \since
 * v.5.5.4
 *
 * \brief Prefix of data sources with statistics for cooperations and
 * agents repository.
 */
SO_5_FUNC prefix_t
coop_repository();

/*!
 * \since
 * v.5.5.4
 *
 * \brief Prefix of data sources with statistics for mboxes repository.
 */
SO_5_FUNC prefix_t
mbox_repository();

/*!
 * \since
 * v.5.5.4
 *
 * \brief Prefix of data sources with statistics for timer thread.
 */
SO_5_FUNC prefix_t
timer_thread();

} /* namespace prefixes */

namespace suffixes {

/*!
 * \since
 * v.5.5.4
 *
 * \brief Suffix for data source with count of registered cooperations.
 */
SO_5_FUNC suffix_t
coop_reg_count();

/*!
 * \since
 * v.5.5.4
 *
 * \brief Suffix for data source with count of cooperations in the
 * deregistration state.
 */
SO_5_FUNC suffix_t
coop_dereg_count();

/*!
 * \since
 * v.5.5.12
 *
 * \brief Suffix for data source with count of cooperations waiting
 * the final deregistration step and removement from coops registry.
 */
SO_5_FUNC suffix_t
coop_final_dereg_count();

/*!
 * \since
 * v.5.5.4
 *
 * \brief Suffix for data source with count of named mboxes.
 */
SO_5_FUNC suffix_t
named_mbox_count();

/*!
 * \since
 * v.5.5.4
 *
 * \brief Suffix for data source with count of agents bound to some entity.
 */
SO_5_FUNC suffix_t
agent_count();

/*!
 * \since
 * v.5.5.4
 *
 * \brief Suffix for data source with count of active groups in
 * an active_group dispatcher.
 */
SO_5_FUNC suffix_t
disp_active_group_count();

/*!
 * \since
 * v.5.5.4
 *
 * \brief Suffix for data source with count of demands in a working thread event queue.
 */
SO_5_FUNC suffix_t
work_thread_queue_size();

/*!
 * \since
 * v.5.5.18
 *
 * \brief Suffix for data source with work thread activity statistics.
 */
SO_5_FUNC suffix_t
work_thread_activity();

/*!
 * \since
 * v.5.5.4
 *
 * \brief Suffix for data source with count of work threads for dispatcher.
 */
SO_5_FUNC suffix_t
disp_thread_count();

/*!
 * \since
 * v.5.5.4
 *
 * \brief Suffix for data source with count of single-shot timers.
 */
SO_5_FUNC suffix_t
timer_single_shot_count();

/*!
 * \since
 * v.5.5.4
 *
 * \brief Suffix for data source with count of periodic timers.
 */
SO_5_FUNC suffix_t
timer_periodic_count();

/*!
 * \since
 * v.5.5.8
 *
 * \brief Suffix for data source with size of quote for demands processing.
 *
 * This suffix is used in prio_one_thread::quoted_round_robin dispatcher.
 */
SO_5_FUNC suffix_t
demand_quote();

} /* namespace suffixes */

} /* namespace stats */

namespace rt {

namespace stats {

namespace prefixes {

/*!
 * \deprecated Will be removed in v.5.6.0.
 */
inline prefix_t
coop_repository() { return so_5::stats::prefixes::coop_repository(); }

/*!
 * \deprecated Will be removed in v.5.6.0.
 */
inline prefix_t
mbox_repository() { return so_5::stats::prefixes::mbox_repository(); }

/*!
 * \deprecated Will be removed in v.5.6.0.
 */
inline prefix_t
timer_thread() { return so_5::stats::prefixes::timer_thread(); }

} /* namespace prefixes */

namespace suffixes {

/*!
 * \deprecated Will be removed in v.5.6.0.
 */
inline suffix_t
coop_reg_count() { return so_5::stats::suffixes::coop_reg_count(); }

/*!
 * \deprecated Will be removed in v.5.6.0.
 */
inline suffix_t
coop_dereg_count() { return so_5::stats::suffixes::coop_dereg_count(); }

/*!
 * \deprecated Will be removed in v.5.6.0.
 */
inline suffix_t
coop_final_dereg_count() { return so_5::stats::suffixes::coop_final_dereg_count(); }

/*!
 * \deprecated Will be removed in v.5.6.0.
 */
inline suffix_t
named_mbox_count() { return so_5::stats::suffixes::named_mbox_count(); }

/*!
 * \deprecated Will be removed in v.5.6.0.
 */
inline suffix_t
agent_count() { return so_5::stats::suffixes::agent_count(); }

/*!
 * \deprecated Will be removed in v.5.6.0.
 */
inline suffix_t
disp_active_group_count() { return so_5::stats::suffixes::disp_active_group_count(); }

/*!
 * \deprecated Will be removed in v.5.6.0.
 */
inline suffix_t
work_thread_queue_size() { return so_5::stats::suffixes::work_thread_queue_size(); }

/*!
 * \deprecated Will be removed in v.5.6.0.
 */
inline suffix_t
disp_thread_count() { return so_5::stats::suffixes::disp_thread_count(); }

/*!
 * \deprecated Will be removed in v.5.6.0.
 */
inline suffix_t
timer_single_shot_count() { return so_5::stats::suffixes::timer_single_shot_count(); }

/*!
 * \deprecated Will be removed in v.5.6.0.
 */
inline suffix_t
timer_periodic_count() { return so_5::stats::suffixes::timer_periodic_count(); }

/*!
 * \deprecated Will be removed in v.5.6.0.
 */
inline suffix_t
demand_quote() { return so_5::stats::suffixes::demand_quote(); }

} /* namespace suffixes */

} /* namespace stats */

} /* namespace rt */

} /* namespace so_5 */


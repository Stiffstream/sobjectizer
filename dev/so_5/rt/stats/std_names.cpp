/*
 * SObjectizer-5
 */

/*!
 * \since v.5.5.4
 * \file
 * \brief Standard data sources prefixes and suffixes used by SObjectizer.
 */

#include <so_5/rt/stats/h/std_names.hpp>

namespace so_5 {

namespace rt {

namespace stats {

SO_5_FUNC prefix_t
prefix_coop_repository()
	{
		return prefix_t( "coop_repository" );
	}

SO_5_FUNC prefix_t
prefix_mbox_repository()
	{
		return prefix_t( "mbox_repository" );
	}

SO_5_FUNC prefix_t
prefix_timer_thread()
	{
		return prefix_t( "timer_thread" );
	}

//
// --- suffixes ---
//

#define IMPL_SUFFIX(val) \
static const char s[] = val;\
return suffix_t{ s };

SO_5_FUNC suffix_t
suffix_coop_reg_count()
	{
		IMPL_SUFFIX( "/coop.reg.count" )
	}

SO_5_FUNC suffix_t
suffix_coop_dereg_count()
	{
		IMPL_SUFFIX( "/coop.dereg.count" )
	}

SO_5_FUNC suffix_t
suffix_named_mbox_count()
	{
		IMPL_SUFFIX( "/named_mbox.count" )
	}

SO_5_FUNC suffix_t
suffix_agent_count()
	{
		IMPL_SUFFIX( "/agent.count" )
	}

SO_5_FUNC suffix_t
suffix_disp_active_group_count()
	{
		IMPL_SUFFIX( "/group.count" )
	}

SO_5_FUNC suffix_t
suffix_work_thread_queue_size()
	{
		IMPL_SUFFIX( "/demands.count" )
	}

SO_5_FUNC suffix_t
suffix_disp_thread_count()
	{
		IMPL_SUFFIX( "/threads.count" )
	}

SO_5_FUNC suffix_t
suffix_timer_single_shot_count()
	{
		IMPL_SUFFIX( "/single_shot.count" )
	}

SO_5_FUNC suffix_t
suffix_timer_periodic_count()
	{
		IMPL_SUFFIX( "/periodic.count" )
	}

#undef IMPL_SUFFIX

} /* namespace stats */

} /* namespace rt */

} /* namespace so_5 */


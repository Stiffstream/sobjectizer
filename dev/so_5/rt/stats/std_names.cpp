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

#include <so_5/rt/stats/h/std_names.hpp>

namespace so_5 {

namespace stats {

namespace prefixes {

SO_5_FUNC prefix_t
coop_repository()
	{
		return prefix_t( "coop_repository" );
	}

SO_5_FUNC prefix_t
mbox_repository()
	{
		return prefix_t( "mbox_repository" );
	}

SO_5_FUNC prefix_t
timer_thread()
	{
		return prefix_t( "timer_thread" );
	}

} /* namespace prefixes */

namespace suffixes {

#define IMPL_SUFFIX(val) \
static const char s[] = val;\
return suffix_t{ s };

SO_5_FUNC suffix_t
coop_reg_count()
	{
		IMPL_SUFFIX( "/coop.reg.count" )
	}

SO_5_FUNC suffix_t
coop_dereg_count()
	{
		IMPL_SUFFIX( "/coop.dereg.count" )
	}

SO_5_FUNC suffix_t
coop_final_dereg_count()
	{
		IMPL_SUFFIX( "/coop.final.dereg.count" )
	}

SO_5_FUNC suffix_t
named_mbox_count()
	{
		IMPL_SUFFIX( "/named_mbox.count" )
	}

SO_5_FUNC suffix_t
agent_count()
	{
		IMPL_SUFFIX( "/agent.count" )
	}

SO_5_FUNC suffix_t
disp_active_group_count()
	{
		IMPL_SUFFIX( "/group.count" )
	}

SO_5_FUNC suffix_t
work_thread_queue_size()
	{
		IMPL_SUFFIX( "/demands.count" )
	}

SO_5_FUNC suffix_t
work_thread_activity()
	{
		IMPL_SUFFIX( "/thread.activity" )
	}

SO_5_FUNC suffix_t
disp_thread_count()
	{
		IMPL_SUFFIX( "/threads.count" )
	}

SO_5_FUNC suffix_t
timer_single_shot_count()
	{
		IMPL_SUFFIX( "/single_shot.count" )
	}

SO_5_FUNC suffix_t
timer_periodic_count()
	{
		IMPL_SUFFIX( "/periodic.count" )
	}

SO_5_FUNC suffix_t
demand_quote()
	{
		IMPL_SUFFIX( "/demands.quote" )
	}

#undef IMPL_SUFFIX

} /* namespace suffixes */

} /* namespace stats */

} /* namespace so_5 */


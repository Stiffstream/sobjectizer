#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target {

	if 'cygwin' != toolset.tag( 'gcc_port', 'NOTGCC' )
		required_prj 'test/so_5/spinlocks/llvm_inspired_test/prj.ut.rb'
		required_prj 'test/so_5/spinlocks/combined_queue_lock/prj.ut.rb'
	end

	required_prj 'test/so_5/details/build_tests.rb'

	required_prj 'test/so_5/environment/moveable_params/prj.ut.rb'
	required_prj 'test/so_5/environment/autoshutdown/prj.ut.rb'
	required_prj 'test/so_5/environment/autoshutdown_in_init/prj.ut.rb'
	required_prj 'test/so_5/environment/autoshutdown_disabled/prj.ut.rb'
	required_prj 'test/so_5/environment/add_disp_after_start/prj.ut.rb'
	required_prj 'test/so_5/environment/reg_coop_after_stop/prj.ut.rb'
	required_prj 'test/so_5/environment/autoname_coop/prj.ut.rb'

	required_prj 'test/so_5/wrapped_env/build_tests.rb'

	required_prj 'test/so_5/execution_hint/basic_checks/prj.ut.rb'

	required_prj( "test/so_5/timer_thread/single_delayed/prj.ut.rb" )
	required_prj( "test/so_5/timer_thread/single_periodic/prj.ut.rb" )
	required_prj( "test/so_5/timer_thread/single_timer_zero_delay/prj.ut.rb" )
	required_prj( "test/so_5/timer_thread/timers_cancelation/prj.ut.rb" )

	required_prj( "test/so_5/disp/binder/bind_to_disp_1/prj.ut.rb" )
	required_prj( "test/so_5/disp/binder/bind_to_disp_2/prj.ut.rb" )
	required_prj( "test/so_5/disp/binder/bind_to_disp_3/prj.ut.rb" )
	required_prj( "test/so_5/disp/binder/bind_to_disp_error_no_disp/prj.ut.rb" )
	required_prj( "test/so_5/disp/binder/bind_to_disp_error_disp_type_mismatch/prj.ut.rb" )
	required_prj( "test/so_5/disp/binder/correct_unbind_after_throw_on_bind/prj.ut.rb" )

	required_prj( "test/so_5/disp/thread_pool/build_tests.rb" )

	required_prj( "test/so_5/disp/adv_thread_pool/build_tests.rb" )

	required_prj( "test/so_5/disp/private_dispatchers/build_tests.rb" )

	required_prj( "test/so_5/disp/prio_ot_strictly_ordered/build_tests.rb" )
	required_prj( "test/so_5/disp/prio_ot_quoted_round_robin/build_tests.rb" )

	required_prj( "test/so_5/disp/prio_dt_one_per_prio/build_tests.rb" )

	required_prj( "test/so_5/event_handler/build_tests.rb" )

	required_prj( "test/so_5/messages/build_tests.rb" )

	required_prj( "test/so_5/state/change_state/prj.ut.rb" )

	required_prj( "test/so_5/coop/build_tests.rb" )

	required_prj( "test/so_5/mbox/build_tests.rb" )

	required_prj( "test/so_5/msg_tracing/build_tests.rb" )

	required_prj( "test/so_5/ad_hoc_agents/build_tests.rb" )

	required_prj( "test/so_5/message_limits/build_tests.rb" )

	required_prj( "test/so_5/layer/build_tests.rb" )

	required_prj( "test/so_5/api/run_so_environment/prj.ut.rb" )

	required_prj( "test/so_5/svc/build_tests.rb" )

	required_prj( "test/so_5/internal_stats/build_tests.rb" )

	required_prj( "test/so_5/bench/ping_pong/prj.rb" )
	required_prj( "test/so_5/bench/same_msg_in_different_states/prj.rb" )
	required_prj( "test/so_5/bench/parallel_send_to_same_mbox/prj.rb" )
	required_prj( "test/so_5/bench/change_state/prj.rb" )
	required_prj( "test/so_5/bench/many_mboxes/prj.rb" )
	required_prj( "test/so_5/bench/thread_pool_disp/prj.rb" )
	required_prj( "test/so_5/bench/no_workload/prj.rb" )
	required_prj( "test/so_5/bench/agent_ring/prj.rb" )

	required_prj( "test/so_5/samples_as_unit_tests/build_tests.rb" )
}

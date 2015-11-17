#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target {

	path = 'test/so_5'

	if 'cygwin' != toolset.tag( 'gcc_port', 'NOTGCC' )
		required_prj "test/so_5/spinlocks/llvm_inspired_test/prj.ut.rb"
	end

	required_prj "#{path}/details/build_tests.rb"

	required_prj "#{path}/environment/moveable_params/prj.ut.rb"
	required_prj "#{path}/environment/autoshutdown/prj.ut.rb"
	required_prj "#{path}/environment/autoshutdown_in_init/prj.ut.rb"
	required_prj "#{path}/environment/autoshutdown_disabled/prj.ut.rb"
	required_prj "#{path}/environment/add_disp_after_start/prj.ut.rb"
	required_prj "#{path}/environment/reg_coop_after_stop/prj.ut.rb"
	required_prj "#{path}/environment/autoname_coop/prj.ut.rb"

	required_prj "#{path}/wrapped_env/build_tests.rb"

	required_prj "#{path}/execution_hint/basic_checks/prj.ut.rb"

	required_prj "#{path}/timer_thread/single_delayed/prj.ut.rb" 
	required_prj "#{path}/timer_thread/single_periodic/prj.ut.rb" 
	required_prj "#{path}/timer_thread/single_timer_zero_delay/prj.ut.rb" 
	required_prj "#{path}/timer_thread/timers_cancelation/prj.ut.rb" 

	required_prj "#{path}/mpsc_queue_traits/build_tests.rb"

	required_prj "#{path}/disp/binder/bind_to_disp_1/prj.ut.rb" 
	required_prj "#{path}/disp/binder/bind_to_disp_2/prj.ut.rb" 
	required_prj "#{path}/disp/binder/bind_to_disp_3/prj.ut.rb" 
	required_prj "#{path}/disp/binder/bind_to_disp_error_no_disp/prj.ut.rb" 
	required_prj "#{path}/disp/binder/bind_to_disp_error_disp_type_mismatch/prj.ut.rb" 
	required_prj "#{path}/disp/binder/correct_unbind_after_throw_on_bind/prj.ut.rb" 

	required_prj "#{path}/disp/thread_pool/build_tests.rb" 

	required_prj "#{path}/disp/adv_thread_pool/build_tests.rb" 

	required_prj "#{path}/disp/private_dispatchers/build_tests.rb" 

	required_prj "#{path}/disp/prio_ot_strictly_ordered/build_tests.rb" 
	required_prj "#{path}/disp/prio_ot_quoted_round_robin/build_tests.rb" 

	required_prj "#{path}/disp/prio_dt_one_per_prio/build_tests.rb" 

	required_prj "#{path}/event_handler/build_tests.rb" 

	required_prj "#{path}/messages/build_tests.rb" 

	required_prj "#{path}/state/change_state/prj.ut.rb" 

	required_prj "#{path}/coop/build_tests.rb" 

	required_prj "#{path}/mbox/build_tests.rb" 

	required_prj "#{path}/msg_tracing/build_tests.rb" 

	required_prj "#{path}/ad_hoc_agents/build_tests.rb" 

	required_prj "#{path}/message_limits/build_tests.rb" 

	required_prj "#{path}/layer/build_tests.rb" 

	required_prj "#{path}/api/run_so_environment/prj.ut.rb" 

	required_prj "#{path}/svc/build_tests.rb" 

	required_prj "#{path}/internal_stats/build_tests.rb" 

	required_prj "#{path}/bench/ping_pong/prj.rb" 
	required_prj "#{path}/bench/same_msg_in_different_states/prj.rb" 
	required_prj "#{path}/bench/parallel_send_to_same_mbox/prj.rb" 
	required_prj "#{path}/bench/change_state/prj.rb" 
	required_prj "#{path}/bench/many_mboxes/prj.rb" 
	required_prj "#{path}/bench/thread_pool_disp/prj.rb" 
	required_prj "#{path}/bench/no_workload/prj.rb" 
	required_prj "#{path}/bench/agent_ring/prj.rb" 
	required_prj "#{path}/bench/coop_dereg/prj.rb" 

	required_prj "#{path}/samples_as_unit_tests/build_tests.rb" 
}

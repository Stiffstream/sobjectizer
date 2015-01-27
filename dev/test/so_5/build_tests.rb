#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target {

	if 'cygwin' != toolset.tag( 'gcc_port', 'NOTGCC' )
		required_prj 'test/so_5/spinlocks/llvm_inspired_test/prj.ut.rb'
	end

	required_prj 'test/so_5/environment/moveable_params/prj.ut.rb'
	required_prj 'test/so_5/environment/autoshutdown/prj.ut.rb'
	required_prj 'test/so_5/environment/autoshutdown_in_init/prj.ut.rb'
	required_prj 'test/so_5/environment/autoshutdown_disabled/prj.ut.rb'
	required_prj 'test/so_5/environment/add_disp_after_start/prj.ut.rb'
	required_prj 'test/so_5/environment/reg_coop_after_stop/prj.ut.rb'
	required_prj 'test/so_5/environment/autoname_coop/prj.ut.rb'

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

	required_prj( "test/so_5/event_handler/subscribe_errors/prj.ut.rb" )
	required_prj( "test/so_5/event_handler/ignore_exception/prj.ut.rb" )
	required_prj( "test/so_5/event_handler/exception_reaction_inheritance/prj.ut.rb" )

	required_prj( "test/so_5/messages/three_messages/prj.ut.rb" )
	required_prj( "test/so_5/messages/resend_message/prj.ut.rb" )
	required_prj( "test/so_5/messages/store_and_resend_later/prj.ut.rb" )

	required_prj( "test/so_5/state/change_state/prj.ut.rb" )

	required_prj( "test/so_5/coop/duplicate_name/prj.ut.rb" )
	required_prj( "test/so_5/coop/reg_some_and_stop_1/prj.ut.rb" )
	required_prj( "test/so_5/coop/reg_some_and_stop_2/prj.ut.rb" )
	required_prj( "test/so_5/coop/reg_some_and_stop_3/prj.ut.rb" )
	required_prj( "test/so_5/coop/throw_on_define_agent/prj.ut.rb" )
	required_prj( "test/so_5/coop/throw_on_bind_to_disp/prj.ut.rb" )
	required_prj( "test/so_5/coop/throw_on_bind_to_disp_2/prj.ut.rb" )
	required_prj( "test/so_5/coop/coop_notify_1/prj.ut.rb" )
	required_prj( "test/so_5/coop/coop_notify_2/prj.ut.rb" )
	required_prj( "test/so_5/coop/coop_notify_3/prj.ut.rb" )
	required_prj( "test/so_5/coop/parent_child_1/prj.ut.rb" )
	required_prj( "test/so_5/coop/parent_child_2/prj.ut.rb" )
	required_prj( "test/so_5/coop/parent_child_3/prj.ut.rb" )
	required_prj( "test/so_5/coop/parent_child_4/prj.ut.rb" )
	required_prj( "test/so_5/coop/user_resource/prj.ut.rb" )

	required_prj( "test/so_5/mbox/subscribe_when_deregistered/prj.ut.rb" )
	required_prj( "test/so_5/mbox/drop_subscription/prj.ut.rb" )
	required_prj( "test/so_5/mbox/drop_subscr_when_demand_in_queue/prj.ut.rb" )
	required_prj( "test/so_5/mbox/mpsc_mbox/prj.ut.rb" )
	required_prj( "test/so_5/mbox/mpsc_mbox_illegal_subscriber/prj.ut.rb" )
	required_prj( "test/so_5/mbox/mpsc_mbox_stress/prj.rb" )
	required_prj( "test/so_5/mbox/hanging_subscriptions/prj.ut.rb" )

	required_prj( "test/so_5/layer/layer_init/prj.ut.rb" )
	required_prj( "test/so_5/layer/layer_query/prj.ut.rb" )
	required_prj( "test/so_5/layer/extra_layer_query/prj.ut.rb" )
	required_prj( "test/so_5/layer/extra_layer_errors/prj.ut.rb" )

	required_prj( "test/so_5/api/run_so_environment/prj.ut.rb" )

	required_prj( "test/so_5/svc/build_tests.rb" )

	required_prj( "test/so_5/bench/ping_pong/prj.rb" )
	required_prj( "test/so_5/bench/same_msg_in_different_states/prj.rb" )
	required_prj( "test/so_5/bench/parallel_send_to_same_mbox/prj.rb" )
	required_prj( "test/so_5/bench/change_state/prj.rb" )
	required_prj( "test/so_5/bench/many_mboxes/prj.rb" )
	required_prj( "test/so_5/bench/thread_pool_disp/prj.rb" )
	required_prj( "test/so_5/bench/no_workload/prj.rb" )
	required_prj( "test/so_5/bench/agent_ring/prj.rb" )
}

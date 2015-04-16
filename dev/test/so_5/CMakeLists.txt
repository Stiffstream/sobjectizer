project(tests)

if( NOT CYGWIN )
  add_subdirectory(spinlocks/llvm_inspired_test)
endif()

add_subdirectory(environment/moveable_params)
add_subdirectory(environment/autoshutdown)
add_subdirectory(environment/autoshutdown_in_init)
add_subdirectory(environment/autoshutdown_disabled)
add_subdirectory(environment/add_disp_after_start)
add_subdirectory(environment/reg_coop_after_stop)
add_subdirectory(environment/autoname_coop)

add_subdirectory(execution_hint/basic_checks)

add_subdirectory(timer_thread/single_delayed)
add_subdirectory(timer_thread/single_periodic)
add_subdirectory(timer_thread/single_timer_zero_delay)
add_subdirectory(timer_thread/timers_cancelation)

add_subdirectory(disp/binder/bind_to_disp_1)
add_subdirectory(disp/binder/bind_to_disp_2)
add_subdirectory(disp/binder/bind_to_disp_3)
add_subdirectory(disp/binder/bind_to_disp_error_no_disp)
add_subdirectory(disp/binder/bind_to_disp_error_disp_type_mismatch)
add_subdirectory(disp/binder/correct_unbind_after_throw_on_bind)

add_subdirectory(disp/thread_pool)

add_subdirectory(disp/adv_thread_pool)

add_subdirectory(disp/private_dispatchers)

add_subdirectory(event_handler/subscribe_errors)
add_subdirectory(event_handler/ignore_exception)
add_subdirectory(event_handler/exception_reaction_inheritance)

add_subdirectory(messages/three_messages)
add_subdirectory(messages/resend_message)
add_subdirectory(messages/store_and_resend_later)

add_subdirectory(state/change_state)

add_subdirectory(coop/duplicate_name)
add_subdirectory(coop/reg_some_and_stop_1)
add_subdirectory(coop/reg_some_and_stop_2)
add_subdirectory(coop/reg_some_and_stop_3)
add_subdirectory(coop/throw_on_define_agent)
add_subdirectory(coop/throw_on_bind_to_disp)
add_subdirectory(coop/throw_on_bind_to_disp_2)
add_subdirectory(coop/coop_notify_1)
add_subdirectory(coop/coop_notify_2)
add_subdirectory(coop/coop_notify_3)
add_subdirectory(coop/parent_child_1)
add_subdirectory(coop/parent_child_2)
add_subdirectory(coop/parent_child_3)
add_subdirectory(coop/parent_child_4)
add_subdirectory(coop/user_resource)

add_subdirectory(mbox/subscribe_when_deregistered)
add_subdirectory(mbox/drop_subscription)
add_subdirectory(mbox/drop_subscr_when_demand_in_queue)
add_subdirectory(mbox/adaptive_subscr_storage)
add_subdirectory(mbox/mpsc_mbox)
add_subdirectory(mbox/mpsc_mbox_illegal_subscriber)
add_subdirectory(mbox/mpsc_mbox_stress)
add_subdirectory(mbox/hanging_subscriptions)

add_subdirectory(ad_hoc_agents)

add_subdirectory(message_limits)

add_subdirectory(layer/layer_init)
add_subdirectory(layer/layer_query)
add_subdirectory(layer/extra_layer_query)
add_subdirectory(layer/extra_layer_errors)

add_subdirectory(api/run_so_environment)

add_subdirectory(svc)

add_subdirectory(bench/ping_pong)
add_subdirectory(bench/same_msg_in_different_states)
add_subdirectory(bench/parallel_send_to_same_mbox)
add_subdirectory(bench/change_state)
add_subdirectory(bench/many_mboxes)
add_subdirectory(bench/thread_pool_disp)
add_subdirectory(bench/no_workload)
add_subdirectory(bench/agent_ring)


project(tests)

if( NOT CYGWIN )
  add_subdirectory(spinlocks/llvm_inspired_test)
endif()

add_subdirectory(environment)

add_subdirectory(wrapped_env)

add_subdirectory(execution_hint/basic_checks)

add_subdirectory(timer_thread)

add_subdirectory(mpsc_queue_traits)

add_subdirectory(disp)

add_subdirectory(event_handler)

add_subdirectory(messages)
add_subdirectory(enveloped_msg)

add_subdirectory(state)

add_subdirectory(coop/duplicate_name)
add_subdirectory(coop/dereg_empty_coop)
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
add_subdirectory(coop/introduce_coop)
add_subdirectory(coop/introduce_coop_2)
add_subdirectory(coop/introduce_coop_3)
add_subdirectory(coop/introduce_coop_4)
add_subdirectory(coop/create_child_coop_5_5_8)

add_subdirectory(mbox)

add_subdirectory(mchain)

add_subdirectory(msg_tracing)

add_subdirectory(message_limits)

add_subdirectory(layer/layer_init)
add_subdirectory(layer/layer_query)
add_subdirectory(layer/extra_layer_query)
add_subdirectory(layer/extra_layer_errors)

add_subdirectory(api/run_so_environment)

add_subdirectory(svc)
add_subdirectory(mutable_msg)

add_subdirectory(internal_stats)

add_subdirectory(env_infrastructure)

add_subdirectory(event_queue_hook)

add_subdirectory(testing)

add_subdirectory(bench/ping_pong)
add_subdirectory(bench/same_msg_in_different_states)
add_subdirectory(bench/parallel_send_to_same_mbox)
add_subdirectory(bench/change_state)
add_subdirectory(bench/many_mboxes)
add_subdirectory(bench/thread_pool_disp)
add_subdirectory(bench/no_workload)
add_subdirectory(bench/agent_ring)
add_subdirectory(bench/coop_dereg)
add_subdirectory(bench/skynet1m)
add_subdirectory(bench/prepared_receive)
add_subdirectory(bench/prepared_select)

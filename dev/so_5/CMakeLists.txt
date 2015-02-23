project(so_5)
add_definitions(-DSO_5_PRJ)

if(WIN32)
	add_definitions(-DSO_5__PLATFORM_REQUIRES_CDECL)
endif()

set(SO_5_SRC exception.cpp
	current_thread_id.cpp
	atomic_refcounted.cpp
	error_logger.cpp
	timers.cpp
	rt/nonempty_name.cpp
	rt/message.cpp
	rt/mbox.cpp
	rt/event_queue.cpp
	rt/temporary_event_queue.cpp
	rt/event_exception_logger.cpp
	rt/agent.cpp
	rt/agent_state_listener.cpp
	rt/adhoc_agent_wrapper.cpp
	rt/agent_coop.cpp
	rt/agent_coop_notifications.cpp
	rt/environment.cpp
	rt/disp.cpp
	rt/disp_binder.cpp
	rt/so_layer.cpp
	rt/coop_listener.cpp

	rt/impl/subscription_storage_iface.cpp
	rt/impl/subscr_storage_vector_based.cpp
	rt/impl/subscr_storage_map_based.cpp
	rt/impl/subscr_storage_hash_table_based.cpp
	rt/impl/subscr_storage_adaptive.cpp
	rt/impl/process_unhandled_exception.cpp
	rt/impl/local_mbox.cpp
	rt/impl/named_local_mbox.cpp
	rt/impl/mpsc_mbox.cpp
	rt/impl/mbox_core.cpp
	rt/impl/agent_core.cpp
	rt/impl/disp_repository.cpp
	rt/impl/layer_core.cpp
	rt/impl/state_listener_controller.cpp

	rt/impl/coop_dereg/coop_dereg_executor_thread.cpp
	rt/impl/coop_dereg/dereg_demand_queue.cpp
	
	disp/reuse/work_thread/work_thread.cpp
	disp/one_thread/pub.cpp
	disp/active_obj/pub.cpp
	disp/active_group/pub.cpp
	disp/thread_pool/pub.cpp
	disp/adv_thread_pool/pub.cpp
)

add_library(so.${SO_5_VERSION} SHARED ${SO_5_SRC})
install(
	TARGETS so.${SO_5_VERSION}
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	RUNTIME DESTINATION bin
)

project(so_5)
add_definitions(-DSO_5_PRJ)

if(WIN32)
	add_definitions(-DSO_5__PLATFORM_REQUIRES_CDECL)
endif()

include(${CMAKE_SOURCE_DIR}/cmake/target.cmake)

set(SO_5_SRC exception.cpp
	exception.cpp
	current_thread_id.cpp
	atomic_refcounted.cpp
	error_logger.cpp
	timers.cpp
	msg_tracing.cpp
	wrapped_env.cpp
	rt/nonempty_name.cpp
	rt/message.cpp
	rt/message_limit.cpp
	rt/mbox.cpp
	rt/mchain.cpp
	rt/event_queue.cpp
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
	rt/impl/named_local_mbox.cpp
	rt/impl/mbox_core.cpp
	rt/impl/agent_core.cpp
	rt/impl/disp_repository.cpp
	rt/impl/layer_core.cpp
	rt/impl/state_listener_controller.cpp
	
	rt/stats/controller.cpp
	rt/stats/repository.cpp
	rt/stats/std_names.cpp

	rt/stats/impl/std_controller.cpp
	rt/stats/impl/ds_agent_core_stats.cpp
	rt/stats/impl/ds_mbox_core_stats.cpp
	rt/stats/impl/ds_timer_thread_stats.cpp
	
	disp/mpsc_queue_traits/pub.cpp
	disp/mpmc_queue_traits/pub.cpp
	disp/reuse/work_thread/work_thread.cpp
	disp/one_thread/pub.cpp
	disp/active_obj/pub.cpp
	disp/active_group/pub.cpp
	disp/thread_pool/pub.cpp
	disp/adv_thread_pool/pub.cpp
	disp/prio_one_thread/strictly_ordered/pub.cpp
	disp/prio_one_thread/quoted_round_robin/pub.cpp
	disp/prio_dedicated_threads/one_per_prio/pub.cpp
)

add_library(${SO_5_TARGET} SHARED ${SO_5_SRC})

add_library(${SO_5_S_TARGET} STATIC ${SO_5_SRC})
target_compile_definitions(${SO_5_S_TARGET}
	PUBLIC -DSO_5_STATIC_LIB
)

set(SO_5_EXT_LIBS )
if( ANDROID )
	list(APPEND SO_5_EXT_LIBS ${ANDROID_LIBCRYSTAX_FILE})
	list(APPEND SO_5_EXT_LIBS ${ANDROID_LIBSTDCXX_FILE})
endif()

foreach(__extLibrary ${SO_5_EXT_LIBS})
	add_custom_command(TARGET ${SO_5_TARGET} PRE_LINK
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${__extLibrary} $<TARGET_FILE_DIR:${SO_5_TARGET}>)
endforeach()

install(
	TARGETS ${SO_5_TARGET} ${SO_5_S_TARGET}
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	RUNTIME DESTINATION bin
)

foreach(__extLibrary ${SO_5_EXT_LIBS})
	install(FILES ${__extLibrary} DESTINATION lib)
endforeach()

file(GLOB_RECURSE ALL_SO_5_HEADERS RELATIVE ${CMAKE_CURRENT_LIST_DIR} *.hpp)

foreach( HEADER_FILE ${ALL_SO_5_HEADERS} )
	if(NOT HEADER_FILE MATCHES "[\\/]impl[\\/]")
		get_filename_component( DIR ${HEADER_FILE} PATH )
		install( FILES ${HEADER_FILE} DESTINATION include/so_5/${DIR} )
	endif()
endforeach()

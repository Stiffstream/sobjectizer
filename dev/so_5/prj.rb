require 'mxx_ru/cpp'

require 'so_5/version'

MxxRu::Cpp::dll_target {

	rtl_mode( Mxx_ru::Cpp::RTL_SHARED )
	implib_path( 'lib' )

	target( 'so.' + So_5::VERSION )

	define( 'SO_5_PRJ' )

	if 'mswin' == toolset.tag( 'target_os' )
		define( 'SO_5__PLATFORM_REQUIRES_CDECL' )
	end

	# ./
	cpp_source 'exception.cpp'
	cpp_source 'current_thread_id.cpp'
	cpp_source 'atomic_refcounted.cpp'

	cpp_source 'error_logger.cpp'

	cpp_source 'timers.cpp'

	cpp_source 'msg_tracing.cpp'

	cpp_source 'wrapped_env.cpp'

	# Run-time.
	sources_root( 'rt' ) {

		cpp_source 'nonempty_name.cpp'

		cpp_source 'message.cpp'

		cpp_source 'message_limit.cpp'

		cpp_source 'mbox.cpp'

		cpp_source 'event_queue.cpp'

		cpp_source 'event_exception_logger.cpp'

		cpp_source 'agent.cpp'
		cpp_source 'agent_state_listener.cpp'
		cpp_source 'adhoc_agent_wrapper.cpp'

		cpp_source 'agent_coop.cpp'
		cpp_source 'agent_coop_notifications.cpp'

		cpp_source 'environment.cpp'

		cpp_source 'disp.cpp'
		cpp_source 'disp_binder.cpp'

		cpp_source 'so_layer.cpp'

		cpp_source 'coop_listener.cpp'

		sources_root( 'impl' ) {
			cpp_source 'subscription_storage_iface.cpp'
			cpp_source 'subscr_storage_vector_based.cpp'
			cpp_source 'subscr_storage_map_based.cpp'
			cpp_source 'subscr_storage_hash_table_based.cpp'
			cpp_source 'subscr_storage_adaptive.cpp'

			cpp_source 'process_unhandled_exception.cpp'

			cpp_source 'named_local_mbox.cpp'
			cpp_source 'mbox_core.cpp'
			cpp_source 'agent_core.cpp'
			cpp_source 'disp_repository.cpp'
			cpp_source 'layer_core.cpp'
			cpp_source 'state_listener_controller.cpp'

			sources_root( 'coop_dereg' ){
				cpp_source 'coop_dereg_executor_thread.cpp'
				cpp_source 'dereg_demand_queue.cpp'
			}
		}

		sources_root( 'stats' ) {
			cpp_source 'controller.cpp'
			cpp_source 'repository.cpp'
			cpp_source 'std_names.cpp'

			sources_root( 'impl' ) {
				cpp_source 'std_controller.cpp'

				cpp_source 'ds_agent_core_stats.cpp'
				cpp_source 'ds_mbox_core_stats.cpp'
				cpp_source 'ds_timer_thread_stats.cpp'
			}
		}
	}

	sources_root( 'disp' ) {
		sources_root( 'reuse' ) {
			sources_root( 'work_thread' ) {
				cpp_source 'work_thread.cpp'
			}
		}

		sources_root( 'one_thread' ) {
			cpp_source 'pub.cpp'
		}

		sources_root( 'active_obj' ) {
			cpp_source 'pub.cpp'
		}

		sources_root( 'active_group' ) {
			cpp_source 'pub.cpp'
		}

		sources_root( 'thread_pool' ) {
			cpp_source 'pub.cpp'
		}

		sources_root( 'adv_thread_pool' ) {
			cpp_source 'pub.cpp'
		}

		sources_root( 'prio_one_thread' ) {
			sources_root( 'strictly_ordered' ) {
				cpp_source 'pub.cpp'
			}
			sources_root( 'quoted_round_robin' ) {
				cpp_source 'pub.cpp'
			}
		}

		sources_root( 'prio_dedicated_threads' ) {
			sources_root( 'one_per_prio' ) {
				cpp_source 'pub.cpp'
			}
		}
	}
}


require 'mxx_ru/cpp'

require 'so_5/version'

module So5

class Prj < MxxRu::Cpp::LibOrDllTarget
	def initialize( a_alias )
		super a_alias, a_alias

		define 'SO_5_PRJ'

		init_dll_block Proc.new {
			rtl_mode Mxx_ru::Cpp::RTL_SHARED
			implib_path 'lib'

			target 'so.' + So5::VERSION

			if ('gcc' == toolset.name || 'clang' == toolset.name) &&
					'mswin' != toolset.tag( 'target_os' )
				compiler_option '-fvisibility=hidden'
				compiler_option '-fvisibility-inlines-hidden'
			end
		}

		init_lib_block Proc.new {
			target_root 'lib'
			target 'so_s.' + So5::VERSION

			define 'SO_5_STATIC_LIB', OPT_UPSPREAD
		}

		if 'mswin' == toolset.tag( 'target_os' )
			define( 'SO_5__PLATFORM_REQUIRES_CDECL' )
		end

		# ./
		cpp_source 'exception.cpp'
		cpp_source 'current_thread_id.cpp'

		cpp_source 'error_logger.cpp'

		cpp_source 'timers.cpp'

		cpp_source 'msg_tracing.cpp'

		cpp_source 'wrapped_env.cpp'

		# Run-time.
		sources_root( 'rt' ) {

			cpp_source 'message.cpp'
			cpp_source 'enveloped_msg.cpp'
			cpp_source 'handler_makers.cpp'

			cpp_source 'event_queue_hook.cpp'

			cpp_source 'message_limit.cpp'

			cpp_source 'mbox.cpp'
			cpp_source 'mchain.cpp'

			cpp_source 'event_exception_logger.cpp'

			cpp_source 'agent.cpp'

			cpp_source 'agent_coop.cpp'
			cpp_source 'agent_coop_notifications.cpp'

			cpp_source 'queue_locks_defaults_manager.cpp'
			cpp_source 'environment.cpp'

			cpp_source 'disp.cpp'
			cpp_source 'disp_binder.cpp'

			cpp_source 'so_layer.cpp'

			sources_root( 'impl' ) {
				cpp_source 'msg_tracing_helpers.cpp'

				cpp_source 'subscription_storage_iface.cpp'
				cpp_source 'subscr_storage_vector_based.cpp'
				cpp_source 'subscr_storage_map_based.cpp'
				cpp_source 'subscr_storage_hash_table_based.cpp'
				cpp_source 'subscr_storage_adaptive.cpp'

				cpp_source 'process_unhandled_exception.cpp'

				cpp_source 'named_local_mbox.cpp'
				cpp_source 'mbox_core.cpp'

				cpp_source 'coop_repository_basis.cpp'

				cpp_source 'disp_repository.cpp'
				cpp_source 'layer_core.cpp'
				cpp_source 'state_listener_controller.cpp'

				cpp_source 'mt_env_infrastructure.cpp'
				cpp_source 'simple_mtsafe_st_env_infrastructure.cpp'
				cpp_source 'simple_not_mtsafe_st_env_infrastructure.cpp'
			}

			sources_root( 'stats' ) {
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
			sources_root( 'mpsc_queue_traits' ) {
				cpp_source 'pub.cpp'
			}

			sources_root( 'mpmc_queue_traits' ) {
				cpp_source 'pub.cpp'
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

		sources_root( 'experimental' ) {
			sources_root( 'testing' ) {
				sources_root( 'v1' ) {
					cpp_source 'all.cpp'
				}
			}
		}
	end # initialize

end # class Prj

class Dll < Prj
	def initialize
		super 'so_5/prj.rb'
		as_dll
	end
end

class Lib < Prj
	def initialize
		super 'so_5/prj_s.rb'
		as_lib
	end
end

end # module So5


require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target {
	example = lambda { |name|
		required_if_present = lambda { |prj| required_prj(prj) if File.exists?(prj) }
		required_if_present[ "sample/so_5/#{name}/prj.rb" ]
		required_if_present[ "sample/so_5/#{name}/prj_s.rb" ]
	}

	example[ 'hello_world' ]
	example[ 'hello_world_simple_not_mtsafe' ]
	example[ 'hello_evt_handler' ]
	example[ 'hello_evt_lambda' ]
	example[ 'hello_all' ]
	example[ 'hello_delay' ]
	example[ 'hello_periodic' ]
	example[ 'chstate' ]
	example[ 'chstate_msg_tracing' ]
	example[ 'selective_msg_tracing' ]
	example[ 'nohandler_msg_tracing' ]
	example[ 'disp' ]
	example[ 'coop_listener' ]
	example[ 'exception_logger' ]
	example[ 'exception_reaction' ]
	example[ 'coop_notification' ]
	example[ 'coop_user_resources' ]
	example[ 'subscriptions' ]
	example[ 'parent_coop' ]
	example[ 'chameneos_simple' ]
	example[ 'chameneos_prealloc_msgs' ]
	example[ 'ping_pong_minimal' ]
	example[ 'ping_pong' ]
	example[ 'ping_pong_with_owner' ]
	example[ 'hardwork_imit' ]
	example[ 'many_timers' ]
	example[ 'custom_error_logger' ]
	example[ 'simple_message_deadline' ]
	example[ 'dispatcher_hello' ]
	example[ 'dispatcher_restarts' ]
	example[ 'dispatcher_for_children' ]
	example[ 'redirect_and_transform' ]
	example[ 'queue_size_stats' ]
	example[ 'delivery_filters' ]
	example[ 'make_pipeline' ]
	example[ 'machine_control' ]
	example[ 'news_board' ]
	example[ 'prio_work_stealing' ]
	example[ 'mchain_handler_formats' ]
	example[ 'mchain_multi_consumers' ]
	example[ 'wrapped_env_demo_2' ]
	example[ 'producer_consumer_mchain' ]

	example[ 'blinking_led' ]
	example[ 'intercom_statechart' ]
	example[ 'state_deep_history' ]

	example[ 'adv_thread_pool_fifo' ]

	example[ 'mchain_select' ]
	example[ 'mchain_fibonacci' ]

	example[ 'wrapped_env_demo_3' ]

	example[ 'mutable_msg_agents' ]
	example[ 'modify_resend_as_immutable' ]
	example[ 'two_handlers' ]

	example[ 'stop_guard' ]

	example[ 'deadletter_handler' ]

	example[ 'make_new_direct_mbox' ]

	example[ 'make_agent_ref' ]
}

require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target {
  path = 'test/so_5/samples_as_unit_tests'

  required_prj "#{path}/chameneos_prealloc_msgs.ut.rb"
  required_prj "#{path}/chameneos_prealloc_msgs-static.ut.rb"

  required_prj "#{path}/chameneos_simple.ut.rb"
  required_prj "#{path}/chameneos_simple-static.ut.rb"

  required_prj "#{path}/chstate_msg_tracing.ut.rb"
  required_prj "#{path}/chstate_msg_tracing-static.ut.rb"

  required_prj "#{path}/selective_msg_tracing.ut.rb"
  required_prj "#{path}/selective_msg_tracing-static.ut.rb"

  required_prj "#{path}/nohandler_msg_tracing.ut.rb"
  required_prj "#{path}/nohandler_msg_tracing-static.ut.rb"

  required_prj "#{path}/coop_listener.ut.rb"
  required_prj "#{path}/coop_listener-static.ut.rb"

  required_prj "#{path}/coop_notification.ut.rb"
  required_prj "#{path}/coop_notification-static.ut.rb"

  required_prj "#{path}/coop_user_resources.ut.rb"
  required_prj "#{path}/coop_user_resources-static.ut.rb"

  required_prj "#{path}/custom_error_logger.ut.rb"
  required_prj "#{path}/custom_error_logger-static.ut.rb"

  required_prj "#{path}/delivery_filters.ut.rb"
  required_prj "#{path}/delivery_filters-static.ut.rb"

  required_prj "#{path}/exception_logger.ut.rb"
  required_prj "#{path}/exception_logger-static.ut.rb"

  required_prj "#{path}/exception_reaction.ut.rb"
  required_prj "#{path}/exception_reaction-static.ut.rb"

  required_prj "#{path}/hello_all.ut.rb"
  required_prj "#{path}/hello_all-static.ut.rb"

  required_prj "#{path}/hello_evt_handler.ut.rb"
  required_prj "#{path}/hello_evt_handler-static.ut.rb"

  required_prj "#{path}/hello_evt_lambda.ut.rb"
  required_prj "#{path}/hello_evt_lambda-static.ut.rb"

  required_prj "#{path}/hello_world.ut.rb"
  required_prj "#{path}/hello_world-static.ut.rb"

  required_prj "#{path}/mchain_handler_formats.ut.rb"
  required_prj "#{path}/mchain_handler_formats-static.ut.rb"

  required_prj "#{path}/mchain_multi_consumers.ut.rb"
  required_prj "#{path}/mchain_multi_consumers-static.ut.rb"

  required_prj "#{path}/mchain_select.ut.rb"
  required_prj "#{path}/mchain_select-static.ut.rb"

  required_prj "#{path}/mchain_svc_req.ut.rb"
  required_prj "#{path}/mchain_svc_req-static.ut.rb"

  required_prj "#{path}/pimpl.ut.rb"
  required_prj "#{path}/pimpl-static.ut.rb"

  required_prj "#{path}/ping_pong_minimal.ut.rb"
  required_prj "#{path}/ping_pong_minimal-static.ut.rb"

  required_prj "#{path}/private_dispatcher_for_children.ut.rb"
  required_prj "#{path}/private_dispatcher_for_children-static.ut.rb"

  required_prj "#{path}/private_dispatcher_hello.ut.rb"
  required_prj "#{path}/private_dispatcher_hello-static.ut.rb"

  required_prj "#{path}/subscriptions.ut.rb"
  required_prj "#{path}/subscriptions-static.ut.rb"

  required_prj "#{path}/svc_exceptions.ut.rb"
  required_prj "#{path}/svc_exceptions-static.ut.rb"

  required_prj "#{path}/svc_hello.ut.rb"
  required_prj "#{path}/svc_hello-static.ut.rb"

  required_prj "#{path}/mutable_msg_agents.ut.rb"
  required_prj "#{path}/mutable_msg_agents-static.ut.rb"

  required_prj "#{path}/modify_resend_as_immutable.ut.rb"
  required_prj "#{path}/modify_resend_as_immutable-static.ut.rb"

  required_prj "#{path}/two_handlers.ut.rb"
  required_prj "#{path}/two_handlers-static.ut.rb"
}

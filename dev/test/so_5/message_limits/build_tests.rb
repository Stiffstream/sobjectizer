#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

path = 'test/so_5/message_limits'

MxxRu::Cpp::composite_target {

	required_prj "#{path}/subscr_without_limit/prj.ut.rb"
	required_prj "#{path}/duplicate_limit/prj.ut.rb"
	required_prj "#{path}/drop/prj.ut.rb"
	required_prj "#{path}/drop_at_peaks/prj.ut.rb"
	required_prj "#{path}/abort_app/mc_mbox/prj.ut.rb"
	required_prj "#{path}/abort_app/sc_mbox/prj.ut.rb"

	required_prj "#{path}/log_then_abort_for_signal/mc_mbox/prj.ut.rb"
	required_prj "#{path}/log_then_abort_for_signal/sc_mbox/prj.ut.rb"
	required_prj "#{path}/log_then_abort_for_msg/mc_mbox/prj.ut.rb"
	required_prj "#{path}/log_then_abort_for_msg/sc_mbox/prj.ut.rb"

	required_prj "#{path}/redirect_msg/mc_mbox/prj.ut.rb"
	required_prj "#{path}/redirect_msg/sc_mbox/prj.ut.rb"
	required_prj "#{path}/default_redirect_msg/mc_mbox/prj.ut.rb"
	required_prj "#{path}/default_redirect_msg/sc_mbox/prj.ut.rb"
	required_prj "#{path}/direct_mbox_delayed_subscription/prj.ut.rb"
	required_prj "#{path}/redirect_msg_too_deep/mc_mbox/prj.ut.rb"
	required_prj "#{path}/redirect_msg_too_deep/sc_mbox/prj.ut.rb"
	required_prj "#{path}/transform_msg/mc_mbox/prj.ut.rb"
	required_prj "#{path}/transform_msg/sc_mbox/prj.ut.rb"
	required_prj "#{path}/transform_msg_too_deep/mc_mbox/prj.ut.rb"
	required_prj "#{path}/transform_msg_too_deep/sc_mbox/prj.ut.rb"
	required_prj "#{path}/transform_to_mutable_msg/prj.ut.rb"
	required_prj "#{path}/transform_to_mutable_msg_2/prj.ut.rb"
	required_prj "#{path}/transform_to_mutable_msg_4/prj.ut.rb"

	required_prj "#{path}/another_direct_mbox/prj.ut.rb"

	required_prj "#{path}/wont_compile_runner/prj.ut.rb"

	required_prj "#{path}/redirect_from_timer_to_full_mchain/prj.ut.rb"
	required_prj "#{path}/transform_from_timer_to_full_mchain/prj.ut.rb"

	required_prj "#{path}/any_unspecified_msg_and_state_time_limit/prj.ut.rb"

	required_prj "#{path}/subscription_unsubscription_1/prj.ut.rb"
}


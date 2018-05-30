#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

path = 'test/so_5/state'

MxxRu::Cpp::composite_target {

	required_prj "#{path}/state_comparison/prj.ut.rb"
	required_prj "#{path}/change_state/prj.ut.rb"
	required_prj "#{path}/composite_state_change/prj.ut.rb"
	required_prj "#{path}/enter_exit_handlers/prj.ut.rb"
	required_prj "#{path}/const_enter_exit_handlers/prj.ut.rb"
	required_prj "#{path}/enter_exit_current_state_ptr/prj.ut.rb"
	required_prj "#{path}/on_exit_on_dereg_1/prj.ut.rb"
	required_prj "#{path}/on_exit_on_dereg_2/prj.ut.rb"
	required_prj "#{path}/nesting_deep/prj.ut.rb"
	required_prj "#{path}/parent_state_handler/prj.ut.rb"
	required_prj "#{path}/suppress_event/prj.ut.rb"
	required_prj "#{path}/state_history/prj.ut.rb"
	required_prj "#{path}/state_history_clear/prj.ut.rb"
	required_prj "#{path}/transfer_to_state/prj.ut.rb"
	required_prj "#{path}/transfer_to_state_loop/prj.ut.rb"
	required_prj "#{path}/just_switch_to/prj.ut.rb"
	required_prj "#{path}/state_switch_guard/prj.ut.rb"
	required_prj "#{path}/time_limit/build_tests.rb"
}

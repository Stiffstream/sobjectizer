#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target {

	path = 'test/so_5/environment/stop_guards'

	required_prj "#{path}/parallel_remove/prj.ut.rb"
	required_prj "#{path}/parallel_remove_50k/prj.ut.rb"
	required_prj "#{path}/simple/prj.ut.rb"
	required_prj "#{path}/some_actions_after_stop/prj.ut.rb"
	required_prj "#{path}/stop_in_progress_nothrow/prj.ut.rb"
	required_prj "#{path}/stop_in_progress_throw/prj.ut.rb"
}

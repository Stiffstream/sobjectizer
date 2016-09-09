#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

path = 'test/so_5/internal_stats'

MxxRu::Cpp::composite_target {

	required_prj "#{path}/simple_turn_on/prj.ut.rb"
	required_prj "#{path}/simple_coop_count/prj.ut.rb"
	required_prj "#{path}/simple_named_mbox_count/prj.ut.rb"
	required_prj "#{path}/simple_timer_thread/prj.ut.rb"
	required_prj "#{path}/simple_work_thread_activity/prj.ut.rb"

	required_prj "#{path}/all_dispatchers/prj.rb"
}

#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target {

	path = 'test/so_5/env_infrastructure/simple_not_mtsafe_st'

	required_prj "#{path}/empty_init_fn/prj.ut.rb"
	required_prj "#{path}/unknown_exception_init_fn/prj.ut.rb"
	required_prj "#{path}/unknown_exception_init_fn_2/prj.ut.rb"
	required_prj "#{path}/unknown_exception_init_fn_3/prj.ut.rb"
	required_prj "#{path}/autoshutdown_disabled/prj.ut.rb"
	required_prj "#{path}/stop_in_init_fn/prj.ut.rb"
	required_prj "#{path}/simple_agent/prj.ut.rb"
	required_prj "#{path}/stop_when_no_load/prj.ut.rb"
	required_prj "#{path}/thread_id/prj.ut.rb"
	required_prj "#{path}/delayed_msg/prj.ut.rb"
	required_prj "#{path}/timer_factories/prj.ut.rb"
	required_prj "#{path}/periodic_msg/prj.ut.rb"
	required_prj "#{path}/stats_on/prj.ut.rb"
	required_prj "#{path}/stats_coop_count/prj.ut.rb"
	required_prj "#{path}/stats_wt_activity/prj.ut.rb"
	required_prj "#{path}/reg_dereg_notificators/prj.ut.rb"
}

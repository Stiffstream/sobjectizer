#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target {

	path = 'test/so_5/environment'

	required_prj "#{path}/autoname_coop/prj.ut.rb"
	required_prj "#{path}/autoshutdown/prj.ut.rb"
	required_prj "#{path}/autoshutdown_disabled/prj.ut.rb"
	required_prj "#{path}/autoshutdown_in_init/prj.ut.rb"
	required_prj "#{path}/moveable_params/prj.ut.rb"
	required_prj "#{path}/reg_coop_after_stop/prj.ut.rb"
	required_prj "#{path}/unknown_exception_run/prj.ut.rb"

	required_prj "#{path}/stop_guards/build_tests.rb"
}

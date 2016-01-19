#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

path = 'test/so_5/state/time_limit'

MxxRu::Cpp::composite_target {

	required_prj "#{path}/simple/prj.ut.rb"
	required_prj "#{path}/reset_limit/prj.ut.rb"
	required_prj "#{path}/many_switches/prj.ut.rb"
	required_prj "#{path}/cancel_on_dereg/prj.ut.rb"
}

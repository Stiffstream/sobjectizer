#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

path = 'test/so_5/disp/prio_dt_one_per_prio'

MxxRu::Cpp::composite_target {

	required_prj "#{path}/simple/prj.ut.rb"
	required_prj "#{path}/contexts/prj.ut.rb"
	required_prj "#{path}/dereg_when_queue_not_empty/prj.ut.rb"
}

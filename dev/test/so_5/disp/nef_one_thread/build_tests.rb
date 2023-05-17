#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

path = 'test/so_5/disp/nef_one_thread'

MxxRu::Cpp::composite_target {

	required_prj "#{path}/simple/prj.ut.rb"
	required_prj "#{path}/custom_work_thread/prj.ut.rb"
}

#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target {

	path = 'test/so_5/disp/one_thread'

	required_prj( "#{path}/custom_work_thread/prj.ut.rb" )
	required_prj( "#{path}/custom_work_thread_2/prj.ut.rb" )
}

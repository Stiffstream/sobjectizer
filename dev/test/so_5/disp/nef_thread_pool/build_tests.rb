#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target {

	path = 'test/so_5/disp/nef_thread_pool'

	required_prj( "#{path}/simple/prj.ut.rb" )
	required_prj( "#{path}/shutdown_on_bad_alloc/prj.ut.rb" )
	required_prj( "#{path}/unique_thread_id/prj.ut.rb" )
}

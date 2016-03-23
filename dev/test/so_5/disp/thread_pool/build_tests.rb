#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target {

	path = 'test/so_5/disp/thread_pool'

	required_prj( "#{path}/simple/prj.ut.rb" )
	required_prj( "#{path}/cooperation_fifo/prj.ut.rb" )
	required_prj( "#{path}/individual_fifo/prj.ut.rb" )
	required_prj( "#{path}/threshold/prj.ut.rb" )
}

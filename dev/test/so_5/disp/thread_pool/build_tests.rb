#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target {

	required_prj( "test/so_5/disp/thread_pool/simple/prj.ut.rb" )
	required_prj( "test/so_5/disp/thread_pool/cooperation_fifo/prj.ut.rb" )
	required_prj( "test/so_5/disp/thread_pool/individual_fifo/prj.ut.rb" )
}

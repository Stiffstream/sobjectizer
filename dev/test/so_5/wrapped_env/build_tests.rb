#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target {

	path = 'test/so_5/wrapped_env'

	required_prj( "#{path}/simple/prj.ut.rb" )
	required_prj( "#{path}/external_stop_then_join/prj.ut.rb" )
	required_prj( "#{path}/add_coop_after_start/prj.ut.rb" )
}

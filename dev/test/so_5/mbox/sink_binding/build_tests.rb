#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

path = 'test/so_5/mbox/sink_binding'

MxxRu::Cpp::composite_target {

	required_prj( "#{path}/simple/prj.ut.rb" )
	required_prj( "#{path}/simple_with_delivery_filter/prj.ut.rb" )
	required_prj( "#{path}/single_sink_clear/prj.ut.rb" )
	required_prj( "#{path}/single_sink_too_deep/prj.ut.rb" )

	required_prj( "#{path}/multi_sink_simple/prj.ut.rb" )
	required_prj( "#{path}/multi_sink_with_delivery_filter/prj.ut.rb" )
}

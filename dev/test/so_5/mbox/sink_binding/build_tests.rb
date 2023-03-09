#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

path = 'test/so_5/mbox/sink_binding'

MxxRu::Cpp::composite_target {

	required_prj( "#{path}/simple/prj.ut.rb" )
}

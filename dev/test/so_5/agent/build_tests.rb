#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target {

	path = 'test/so_5/agent'

	required_prj( "#{path}/agent_name/prj.ut.rb" )
}


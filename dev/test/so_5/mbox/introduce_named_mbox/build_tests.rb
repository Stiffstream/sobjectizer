#!/usr/bin/ruby
require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target {
	path = 'test/so_5/mbox/introduce_named_mbox'

	required_prj( "#{path}/basic/prj.ut.rb" )
}


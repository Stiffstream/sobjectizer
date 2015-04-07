#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

path = 'test/so_5/layer'

MxxRu::Cpp::composite_target {

	required_prj "#{path}/layer_init/prj.ut.rb"
	required_prj "#{path}/layer_query/prj.ut.rb"
	required_prj "#{path}/extra_layer_query/prj.ut.rb"
	required_prj "#{path}/extra_layer_errors/prj.ut.rb"
}

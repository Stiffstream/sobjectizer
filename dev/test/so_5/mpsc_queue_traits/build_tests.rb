#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target {

	path = 'test/so_5/mpsc_queue_traits'

	required_prj "#{path}/locks/prj.ut.rb"
	required_prj "#{path}/agent_ring/prj.ut.rb"
}

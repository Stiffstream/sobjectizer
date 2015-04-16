#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

path = 'test/so_5/ad_hoc_agents'

MxxRu::Cpp::composite_target {

	required_prj "#{path}/default_exception_reaction/prj.ut.rb"
}

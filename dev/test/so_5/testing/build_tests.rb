#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target {

	path = 'test/so_5/testing'

	required_prj "#{path}/v1/build_tests.rb"
}

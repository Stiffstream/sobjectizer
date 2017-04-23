#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target {

	path = 'test/so_5/env_infrastructure'

	required_prj "#{path}/simple_mtsafe_st/build_tests.rb"
	required_prj "#{path}/simple_not_mtsafe_st/build_tests.rb"
}

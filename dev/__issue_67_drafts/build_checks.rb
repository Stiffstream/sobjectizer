#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target {

	path = '__issue_67_drafts'

	required_prj "#{path}/check_1.rb"
	required_prj "#{path}/check_2.rb"
	required_prj "#{path}/check_3.rb"
}

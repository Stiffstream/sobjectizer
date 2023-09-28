#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target {

	path = '__issue_67_drafts'

	required_prj "#{path}/check_1.rb"
	required_prj "#{path}/check_1_1.rb"
	required_prj "#{path}/check_1_2.rb"

	required_prj "#{path}/check_2.rb"
	required_prj "#{path}/check_2_1.rb"

	required_prj "#{path}/check_3.rb"
	required_prj "#{path}/check_3_1.rb"
	required_prj "#{path}/check_3_2.rb"
}

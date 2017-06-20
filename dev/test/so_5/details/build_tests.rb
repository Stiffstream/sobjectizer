#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target {

	path = 'test/so_5/details'

	required_prj( "#{path}/invoke_noexcept_code/prj.ut.rb" )
	required_prj( "#{path}/remaining_time_counter/prj.ut.rb" )
	required_prj( "#{path}/lock_holder_detector/prj.ut.rb" )
}

#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

path = 'test/so_5/event_handler'

MxxRu::Cpp::composite_target {

	required_prj( "#{path}/subscribe_errors/prj.ut.rb" )
	required_prj( "#{path}/ignore_exception/prj.ut.rb" )
	required_prj( "#{path}/exception_reaction_inheritance/prj.ut.rb" )
	required_prj( "#{path}/subscribe_before_reg/prj.ut.rb" )
}

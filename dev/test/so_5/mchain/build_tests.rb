#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target {

	path = 'test/so_5/mchain'

	required_prj( "#{path}/infinite_wait/prj.ut.rb" )
	required_prj( "#{path}/simple/prj.ut.rb" )
	required_prj( "#{path}/simple_svc/prj.ut.rb" )
	required_prj( "#{path}/timers/prj.ut.rb" )
	required_prj( "#{path}/close_chain/prj.ut.rb" )
	required_prj( "#{path}/limited_no_app_abort/prj.ut.rb" )
	required_prj( "#{path}/limited_app_abort/prj.ut.rb" )
	required_prj( "#{path}/adv_receive/prj.ut.rb" )
	required_prj( "#{path}/not_empty_notify/prj.ut.rb" )
}

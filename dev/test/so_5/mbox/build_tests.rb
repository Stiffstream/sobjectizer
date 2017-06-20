#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

path = 'test/so_5/mbox'

MxxRu::Cpp::composite_target {

	required_prj( "#{path}/subscribe_when_deregistered/prj.ut.rb" )
	required_prj( "#{path}/drop_subscription/prj.ut.rb" )
	required_prj( "#{path}/drop_subscr_when_demand_in_queue/prj.ut.rb" )
	required_prj( "#{path}/adaptive_subscr_storage/prj.ut.rb" )
	required_prj( "#{path}/mpsc_mbox/prj.ut.rb" )
	required_prj( "#{path}/mpsc_mbox_illegal_subscriber/prj.ut.rb" )
	required_prj( "#{path}/mpsc_mbox_stress/prj.ut.rb" )
	required_prj( "#{path}/hanging_subscriptions/prj.ut.rb" )
	required_prj( "#{path}/delivery_filters/build_tests.rb" )
	required_prj( "#{path}/local_mbox_growth/prj.ut.rb" )
	required_prj( "#{path}/custom_mbox_simple/prj.ut.rb" )
}

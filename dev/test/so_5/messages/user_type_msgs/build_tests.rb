#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target {

	path = 'test/so_5/messages/user_type_msgs'

	required_prj( "#{path}/simple_msgs/prj.ut.rb" )
	required_prj( "#{path}/different_handlers/prj.ut.rb" )
	required_prj( "#{path}/simple_svc/prj.ut.rb" )
	required_prj( "#{path}/different_svc_handlers/prj.ut.rb" )
	required_prj( "#{path}/delivery_filters/prj.ut.rb" )
	required_prj( "#{path}/limit_transform/prj.ut.rb" )
	required_prj( "#{path}/event_data/prj.ut.rb" )
}

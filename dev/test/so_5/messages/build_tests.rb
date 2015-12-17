#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target {

	path = 'test/so_5/messages'

	required_prj( "#{path}/three_messages/prj.ut.rb" )
	required_prj( "#{path}/resend_message/prj.ut.rb" )
	required_prj( "#{path}/store_and_resend_later/prj.ut.rb" )
	required_prj( "#{path}/lambda_handlers/prj.ut.rb" )
	required_prj( "#{path}/tuple_as_message/prj.ut.rb" )
	required_prj( "#{path}/typed_mtag/prj.ut.rb" )

	required_prj( "#{path}/user_type_msgs/build_tests.rb" )
}

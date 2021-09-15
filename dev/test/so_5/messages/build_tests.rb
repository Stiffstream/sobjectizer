#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target {

	path = 'test/so_5/messages'

	required_prj( "#{path}/three_messages/prj.ut.rb" )
	required_prj( "#{path}/message_holder/prj.ut.rb" )
	required_prj( "#{path}/resend_message/prj.ut.rb" )
	required_prj( "#{path}/resend_message_2/prj.ut.rb" )
	required_prj( "#{path}/resend_message_as_mutable/prj.ut.rb" )
	required_prj( "#{path}/resend_message_as_mutable_2/prj.ut.rb" )
	required_prj( "#{path}/store_and_resend_later/prj.ut.rb" )
	required_prj( "#{path}/lambda_handlers/prj.ut.rb" )
	required_prj( "#{path}/signal_redirection/prj.ut.rb" )

	required_prj( "#{path}/user_type_msgs/build_tests.rb" )
}

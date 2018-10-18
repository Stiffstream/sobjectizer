#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target {

	path = 'test/so_5/enveloped_msg/message_limits'

	required_prj "#{path}/transform_normal/prj.ut.rb"
	required_prj "#{path}/transform_normal_user_msg/prj.ut.rb"
	required_prj "#{path}/transform_signal/prj.ut.rb"
	required_prj "#{path}/transform_svc_req/prj.ut.rb"
}

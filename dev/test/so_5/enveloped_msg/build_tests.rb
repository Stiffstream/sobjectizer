#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target {

	path = 'test/so_5/enveloped_msg'

	required_prj "#{path}/simplest/prj.ut.rb"
	required_prj "#{path}/simple_svc_req/prj.ut.rb"
	required_prj "#{path}/adv_thread_pool/prj.ut.rb"
	required_prj "#{path}/simple_timer/prj.ut.rb"
	required_prj "#{path}/simple_delivery_filter/prj.ut.rb"

	required_prj "#{path}/simple_mchain_delivery/prj.ut.rb"
	required_prj "#{path}/simple_mchain_timer/prj.ut.rb"
	required_prj "#{path}/mchain_handled_count/prj.ut.rb"

	required_prj "#{path}/message_limits/build_tests.rb"
}

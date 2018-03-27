#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

path = 'test/so_5/msg_tracing'

MxxRu::Cpp::composite_target {

	required_prj "#{path}/simple_msg_count/prj.ut.rb"
	required_prj "#{path}/simple_svc_count/prj.ut.rb"
	required_prj "#{path}/simple_svc_count_on_exception/prj.ut.rb"
	required_prj "#{path}/simple_msg_count_mpsc_no_limits/prj.ut.rb"
	required_prj "#{path}/simple_msg_count_mpsc_limits/prj.ut.rb"

	required_prj "#{path}/overlimit_abort_app/prj.ut.rb"
	required_prj "#{path}/overlimit_drop/prj.ut.rb"
	required_prj "#{path}/overlimit_redirect/prj.ut.rb"
	required_prj "#{path}/overlimit_transform/prj.ut.rb"

	required_prj "#{path}/simple_deny_all_filter/prj.ut.rb"
	required_prj "#{path}/simple_deny_msg_filter/prj.ut.rb"
	required_prj "#{path}/overlimit_redirect_with_filter/prj.ut.rb"
	required_prj "#{path}/change_filter_1/prj.ut.rb"
}

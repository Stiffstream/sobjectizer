#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target {

	path = 'test/so_5/mutable_msg'

	required_prj "#{path}/wont_compile_runner/prj.ut.rb"
	required_prj "#{path}/receive_immutable/prj.ut.rb"
	required_prj "#{path}/receive_mutable/prj.ut.rb"
	required_prj "#{path}/receive_mutable_lambda/prj.ut.rb"
	required_prj "#{path}/mutable_to_mpmc_mbox/prj.ut.rb"
	required_prj "#{path}/mutable_redirect/prj.ut.rb"
	required_prj "#{path}/mutable_to_immutable/prj.ut.rb"
	required_prj "#{path}/delayed_mutable/prj.ut.rb"
	required_prj "#{path}/delayed_mutable_to_immutable/prj.ut.rb"
	required_prj "#{path}/delayed_to_mpmc_mbox/prj.ut.rb"
	required_prj "#{path}/periodic/prj.ut.rb"
	required_prj "#{path}/periodic_to_mpmc_mbox/prj.ut.rb"
	required_prj "#{path}/delayed_mutable_with_timer_id/prj.ut.rb"
	required_prj "#{path}/simple_svc/prj.ut.rb"
	required_prj "#{path}/immutable_svc/prj.ut.rb"
	required_prj "#{path}/simple_mchain/prj.ut.rb"
	required_prj "#{path}/svc_redirect/prj.ut.rb"
	required_prj "#{path}/svc_redirect_2/prj.ut.rb"
	required_prj "#{path}/svc_redirect_signals/prj.ut.rb"
	required_prj "#{path}/svc_redirect_signals_2/prj.ut.rb"
	required_prj "#{path}/subscr_to_mutable_from_mpmc_mbox/prj.ut.rb"
}

#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target {

	path = 'test/so_5/timer_thread'

	required_prj "#{path}/single_delayed/prj.ut.rb" 
	required_prj "#{path}/single_periodic/prj.ut.rb" 
	required_prj "#{path}/single_timer_zero_delay/prj.ut.rb" 
	required_prj "#{path}/timers_cancelation/prj.ut.rb" 
	required_prj "#{path}/overloaded_mchain/prj.ut.rb" 
	required_prj "#{path}/overloaded_mchain_2/prj.ut.rb" 
	required_prj "#{path}/resend_periodic_signal_via_mhood/prj.ut.rb" 
	required_prj "#{path}/resend_periodic_via_mhood_to_mchain/prj.ut.rb" 
	required_prj "#{path}/resend_delayed_via_mhood_to_mchain/prj.ut.rb" 
	required_prj "#{path}/negative_args/prj.ut.rb" 
}

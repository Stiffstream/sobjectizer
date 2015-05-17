#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

path = 'test/so_5/mbox/delivery_filters'

MxxRu::Cpp::composite_target {

	required_prj "#{path}/simple/prj.ut.rb"
	required_prj "#{path}/filter_no_subscriptions/prj.ut.rb"
	required_prj "#{path}/filter_on_mpsc_mbox/prj.ut.rb"
	required_prj "#{path}/dereg_subscriber/prj.ut.rb"
	required_prj "#{path}/set_unset/prj.ut.rb"
	required_prj "#{path}/exception_in_filter/prj.ut.rb"
	required_prj "#{path}/simple_svc/prj.ut.rb"
}

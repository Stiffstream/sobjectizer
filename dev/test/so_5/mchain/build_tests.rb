#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target {

	path = 'test/so_5/mchain'

	required_prj( "#{path}/infinite_wait/prj.ut.rb" )
	required_prj( "#{path}/simple/prj.ut.rb" )
	required_prj( "#{path}/simple_svc/prj.ut.rb" )
	required_prj( "#{path}/func_as_handler/prj.ut.rb" )
	required_prj( "#{path}/timers/prj.ut.rb" )
	required_prj( "#{path}/close_chain/prj.ut.rb" )
	required_prj( "#{path}/limited_no_app_abort/prj.ut.rb" )
	required_prj( "#{path}/limited_app_abort/prj.ut.rb" )
	required_prj( "#{path}/adv_receive/prj.ut.rb" )
	required_prj( "#{path}/adv_prepared_receive/prj.ut.rb" )
	required_prj( "#{path}/not_empty_notify/prj.ut.rb" )
	required_prj( "#{path}/multithread_receive/prj.ut.rb" )
	required_prj( "#{path}/multithread_receive_close/prj.ut.rb" )

	required_prj( "#{path}/select_simple/prj.ut.rb" )
	required_prj( "#{path}/prepared_select_simple/prj.ut.rb" )
	required_prj( "#{path}/select_simple_close/prj.ut.rb" )
	required_prj( "#{path}/select_count_messages/prj.ut.rb" )
	required_prj( "#{path}/select_mthread_close/prj.ut.rb" )
	required_prj( "#{path}/select_mthread_read/prj.ut.rb" )
	required_prj( "#{path}/adv_select_mthread_close/prj.ut.rb" )
	required_prj( "#{path}/adv_select_mthread_read/prj.ut.rb" )
	required_prj( "#{path}/adv_select_mthread_empty_timeout/prj.ut.rb" )
	required_prj( "#{path}/adv_select_mthread_stop_on/prj.ut.rb" )
	required_prj( "#{path}/auto_close_chains/prj.ut.rb" )
	required_prj( "#{path}/auto_close_chains_ex/prj.ut.rb" )
	required_prj( "#{path}/master_handle/prj.ut.rb" )

	required_prj( "#{path}/receive_closed_handler/prj.ut.rb" )
	required_prj( "#{path}/select_closed_handler/prj.ut.rb" )
}

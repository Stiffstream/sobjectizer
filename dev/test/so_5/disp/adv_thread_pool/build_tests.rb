#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target {

	required_prj( "test/so_5/disp/adv_thread_pool/simple/prj.ut.rb" )
	required_prj( "test/so_5/disp/adv_thread_pool/chstate_in_safe/prj.ut.rb" )
	required_prj( "test/so_5/disp/adv_thread_pool/subscr_in_safe/prj.ut.rb" )
	required_prj( "test/so_5/disp/adv_thread_pool/chained_svc_call/prj.ut.rb" )
	required_prj( "test/so_5/disp/adv_thread_pool/chained_svc_call_adhoc/prj.ut.rb" )
	required_prj( "test/so_5/disp/adv_thread_pool/cooperation_fifo/prj.ut.rb" )
	required_prj( "test/so_5/disp/adv_thread_pool/individual_fifo/prj.ut.rb" )
	required_prj( "test/so_5/disp/adv_thread_pool/unsafe_after_safe/prj.ut.rb" )
}

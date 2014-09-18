#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target {

	required_prj( "test/so_5/svc/stable_proxy/prj.ut.rb" )
	required_prj( "test/so_5/svc/simple_svc/prj.ut.rb" )
	required_prj( "test/so_5/svc/simple_lambda_svc/prj.ut.rb" )
	required_prj( "test/so_5/svc/simple_mutable_lambda_svc/prj.ut.rb" )
	required_prj( "test/so_5/svc/simple_svc_adhoc_agents/prj.ut.rb" )
	required_prj( "test/so_5/svc/svc_handler_exception/prj.ut.rb" )
	required_prj( "test/so_5/svc/no_svc_handlers/prj.ut.rb" )
	required_prj( "test/so_5/svc/several_svc_handlers/prj.ut.rb" )
	required_prj( "test/so_5/svc/svc_handler_not_called/prj.ut.rb" )
	required_prj( "test/so_5/svc/resending/prj.ut.rb" )
	required_prj( "test/so_5/svc/resending_sync_request/prj.ut.rb" )

	if not ("vc" == toolset.name and 11 <= toolset.tag( 'ver_hi' ).to_i)
		required_prj( "test/so_5/svc/make_sync_request/prj.ut.rb" )
		required_prj( "test/so_5/svc/sync_request_and_wait_for/prj.ut.rb" )
	end
}
